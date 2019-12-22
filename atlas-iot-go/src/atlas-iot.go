package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"math"
	"net/http"
	"net/http/httputil"
	"os"
	"time"

	"github.com/gorilla/mux"
	"go.mongodb.org/mongo-driver/bson/primitive"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

// See https://en.wikipedia.org/wiki/Dew_point

const port int = 60135
const absoluteZeroC = -273.15

// Dew point constants
const a float64 = 6.1121 // mbar
const b float64 = 18.678 // NA
const c float64 = 257.14 // °C
const d float64 = 234.5  // °C

// Heat index constants
const c1 float64 = -8.78469475556
const c2 float64 = 1.61139411
const c3 float64 = 2.33854883889
const c4 float64 = -0.14611605
const c5 float64 = -0.012308094
const c6 float64 = -0.016424827778
const c7 float64 = 0.002211732
const c8 float64 = 0.00072546
const c9 float64 = -0.000003582

func gamma(t float64, h float64) float64 {
	return math.Log(h/100.0) + (b*t)/(c+t)
}

// Get temperature of the dew point in °C
func dp(t float64, h float64) float64 {
	g := gamma(t, h)
	return c * g / (b - g)
}

// Get the saturated water vapor pressure
func ps(t float64, h float64) float64 {
	return a * math.Exp((b*t)/(c+t))
}

// Get the actual vapor pressure
func pa(t float64, h float64) float64 {
	return (h / 100.0) * ps(t, h)
}

// Get the heat index
func hi(t float64, h float64) float64 {
	t2 := t * t
	h2 := h * h
	return c1 + c2*t + c3*h + c4*t*h + c5*t2 + c6*h2 + c7*t2*h + c8*t*h2 + c9*t2*h2
}

var client *mongo.Client

// Reading data structure
type Reading struct {
	Sensor      string  `json:"sensor,omitempty" bson:"sensor,omitempty"`
	Humidity    float32 `json:"humidity,omitempty" bson:"humidity,omitempty"`
	Temperature float32 `json:"temperature,omitempty" bson:"temperature,omitempty"`
	HeatIndex   float32 `json:"heatindex,omitempty" bson:"heatindex,omitempty"`
	DewPoint    float32 `json:"dewpoint,omitempty" bson:"dewpoint,omitempty"`
	Pressure    float32 `json:"pressure,omitempty" bson:"pressure,omitempty"`
	Altitude    float32 `json:"altitude,omitempty" bson:"altitude,omitempty"`
	dp          float32 `json:"dp,omitempty" bson:"dp,omitempty"`
	ps          float32 `json:"ps,omitempty" bson:"ps,omitempty"`
	pa          float32 `json:"pa,omitempty" bson:"pa,omitempty"`
	hi          float32 `json:"hi,omitempty" bson:"hi,omitempty"`
}

// Event data
type Event struct {
	ID       primitive.ObjectID `json:"_id,omitempty" bson:"_id,omitempty"`
	Sender   string             `json:"sender,omitempty" bson:"sender,omitempty"`
	Event    string             `json:"event,omitempty" bson:"event,omitempty"`
	Date     time.Time          `json:"date,omitempty" bson:"date,omitempty"`
	Readings []Reading          `json:"readings,omitempty" bson:"readings,omitempty"`
	Version  string             `json:"version,omitempty" bson:"version,omitempty"`
}

// RequestDump dumps HTTP request data
func RequestDump(request *http.Request) {
	requestDump, err := httputil.DumpRequest(request, true)
	if err != nil {
		log.Println(err)
	}
	log.Println(string(requestDump))
}

// InsertRecordEndpoint is the insert endpoint for requested data
func InsertRecordEndpoint(response http.ResponseWriter, request *http.Request) {
	response.Header().Set("content-type", "application/json")

	RequestDump(request)

	var event Event
	_ = json.NewDecoder(request.Body).Decode(&event)
	event.Date = time.Now()
	log.Println(event)

	n := len(event.Readings)
	if n > 0 {

		for i := 0; i < n; i++ {
			reading := event.Readings[i]
			if reading.Temperature < absoluteZeroC {
				reading.Temperature = absoluteZeroC
				reading.DewPoint = absoluteZeroC
				reading.HeatIndex = absoluteZeroC
			}

			if reading.Humidity < 0 || reading.Humidity > 100 {
				reading.Humidity = 0
				reading.DewPoint = absoluteZeroC
				reading.HeatIndex = absoluteZeroC
			}

			if reading.Humidity >= 0 && reading.Temperature > absoluteZeroC {
				t := (float64)(reading.Temperature)
				h := (float64)(reading.Humidity)
				reading.dp = (float32)((int)(dp(t, h)*100)) / 100.0
				reading.ps = (float32)((int)(ps(t, h)*100)) / 100.0
				reading.pa = (float32)((int)(pa(t, h)*100)) / 100.0
				reading.hi = (float32)((int)(hi(t, h)*100)) / 100.0
			}
		}

		collection := client.Database("iot").Collection("events")
		ctx, _ := context.WithTimeout(context.Background(), 5*time.Second)
		result, _ := collection.InsertOne(ctx, event)

		json.NewEncoder(response).Encode(result)
	} else {
		log.Println("Invalid readings")
		json.NewEncoder(response).Encode(nil)
	}
}

func main() {
	fmt.Println("Starting the application...")

	if len(os.Args) < 2 {
		log.Fatal("Usage: atlas-iot <mongo-db-uri>")
	}

	ctx, _ := context.WithTimeout(context.Background(), 10*time.Second)

	uri := os.Args[1]
	clientOptions := options.Client().ApplyURI(uri)

	var e error
	client, e = mongo.Connect(ctx, clientOptions)
	if e != nil {
		log.Fatal(e)
	}

	router := mux.NewRouter()
	router.HandleFunc("/api/v1/events", InsertRecordEndpoint).Methods("POST")

	fmt.Printf("Start at port %d...\n", port)

	log.Fatal(http.ListenAndServe(fmt.Sprintf(":%d", port), router))

	fmt.Println("After all...")
}
