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

var client *mongo.Client

// Reading data structure
type Reading struct {
	Sensor      string  `json:"sensor,omitempty" bson:"sensor,omitempty"`
	Humidity    float32 `json:"humidity" bson:"humidity"`
	Temperature float32 `json:"temperature" bson:"temperature"`
	HeatIndex   float32 `json:"heatindex" bson:"heatindex"`
	DewPoint    float32 `json:"dewpoint" bson:"dewpoint"`
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

	collection := client.Database("iot").Collection("events")
	ctx, _ := context.WithTimeout(context.Background(), 5*time.Second)
	result, _ := collection.InsertOne(ctx, event)

	json.NewEncoder(response).Encode(result)
}

func main() {
	fmt.Println("Starting the application...")

	if len(os.Args) < 2 {
		log.Fatal("Usage: atlas-iot <mongo-db-uri>")
	}

	ctx, _ := context.WithTimeout(context.Background(), 10*time.Second)

	uri := os.Args[1]
	clientOptions := options.Client().ApplyURI(uri)

	client, _ = mongo.Connect(ctx, clientOptions)

	router := mux.NewRouter()
	router.HandleFunc("/api/v1/events", InsertRecordEndpoint).Methods("POST")

	fmt.Printf("Start at port %d...\n", port)

	log.Fatal(http.ListenAndServe(fmt.Sprintf(":%d", port), router))

	fmt.Println("After all...")
}
