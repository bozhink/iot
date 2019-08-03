"use strict";

module.exports = function (app) {
    var eventLog = require("../controllers/eventLogController");

    // event log routes
    app.route("/api/v1/event")
        .post(eventLog.logEvent);
};
