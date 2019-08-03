"use strict";

var mongoose = require("mongoose");
var Schema = mongoose.Schema;

var AirReadingSchema = new Schema({
    sensor: {
        type: String,
        required: "Enter the name/model of the sensor"
    },
    humidity: {
        type: Number,
        max: 100,
        min: 0
    },
    temperature: {
        type: Number
    },
    heatIndex: {
        type: Number
    },
    dewPoint: {
        type: Number
    }
});

var SoilReadingSchema = new Schema({
    sensor: {
        type: String,
        required: "Enter the name/model of the sensor"
    },
    humidity: {
        type: Number
    }
});

var EventEntry = new Schema({
    sender: {
        type: String,
        required: "Enter the name of the sender/device"
    },
    event: {
        type: String,
    },
    date: {
        type: Date,
        default: Date.now,
    },
    airReadings: [AirReadingSchema],
    soilReadings: [SoilReadingSchema]
});

module.exports = mongoose.model("EventEntry", EventEntry);
