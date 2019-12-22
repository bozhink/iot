package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"net/http/httputil"
	"os"
	"time"

	"github.com/gorilla/mux"
	"go.mongodb.org/mongo-driver/bson/primitive"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

const port int = 60135
const absoluteZeroC = -273.15

var client *mongo.Client

// Reading data structure
type Reading struct {
	Sensor      string   `json:"sensor,omitempty" bson:"sensor,omitempty"`
	Humidity    *float32 `json:"humidity,omitempty" bson:"humidity,omitempty"`
	Temperature *float32 `json:"temperature,omitempty" bson:"temperature,omitempty"`
	HeatIndex   *float32 `json:"heatindex,omitempty" bson:"heatindex,omitempty"`
	DewPoint    *float32 `json:"dewpoint,omitempty" bson:"dewpoint,omitempty"`
	Pressure    *float32 `json:"pressure,omitempty" bson:"pressure,omitempty"`
	Altitude    *float32 `json:"altitude,omitempty" bson:"altitude,omitempty"`
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
			if *reading.Temperature < absoluteZeroC {
				reading.Temperature = nil
				reading.DewPoint = nil
				reading.HeatIndex = nil
			}

			if *reading.Humidity < 0 || *reading.Humidity > 100 {
				reading.Humidity = nil
				reading.DewPoint = nil
				reading.HeatIndex = nil
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
