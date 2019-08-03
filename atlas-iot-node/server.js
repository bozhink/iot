var express = require("express"),
    app = express(),
    port = process.env.PORT || 13027,
    mongoose = require("mongoose"),
    EventLog = require("./api/models/eventLogModel"),
    bodyParser = require("body-parser");

// to fix all deprecation warnings, follow the below steps
// see https://mongoosejs.com/docs/deprecations.html
mongoose.set("useNewUrlParser", true);
mongoose.set("useFindAndModify", false);
mongoose.set("useCreateIndex", true);

// mongoose instance connection url
mongoose.Promise = global.Promise;
mongoose.connect("mongodb://localhost:27017/test?retryWrites=true&w=majority"); // TODO: add Atlas MongoDB URI

app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());

var routes = require("./api/routes/eventLogRoutes");
routes(app);

app.use(function (req, res) {
    //console.log(req);
    res.status(404).send({ url: req.originalUrl + " not found" });
});

app.listen(port);

console.log("event log RESTful API server started on: " + port);

