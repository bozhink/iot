"use strict";

var mongoose = require("mongoose"),
    EventEntry = mongoose.model("EventEntry");

exports.logEvent = function (req, res) {
    var newEventEntry = new EventEntry(req.body);
    newEventEntry.save(function (err, entry) {
        if (err) {
            res.send(err);
        }

        res.json(entry);
    });
}