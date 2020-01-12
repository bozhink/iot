// Connect NodeMCU with DHT11 temperature-and-humidity sensor to MongoDB Stitch application.

#include <Arduino.h>
#include <math.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "DHT.h"

// Dew point constants
const float a = 6.1121; // mbar
const float b = 18.678; // NA
const float c = 257.14; // °C
const float d = 234.5;  // °C

// Fingerprint for URL
// echo -n | openssl s_client -connect eu-west-1.aws.webhooks.mongodb-stitch.com:443 -CAfile /usr/share/ca-certificates/mozilla/DigiCert_Assured_ID_Root_CA.crt | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' > ./x.pem
// openssl x509 -noout -in x.pem -fingerprint -sha1
const uint8_t fingerprint[20] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#ifndef STASSID
#define STASSID "ssid"
#define STAPSK  "password"
#endif

#ifndef APIURI
#define APIURI "https://eu-west-1.aws.webhooks.mongodb-stitch.com/api/client/v2.0/app/{your-app-name}/service/iotc/incoming_webhook/w?secret={your-secret}"
#endif

#ifndef USER_AGENT
#define USER_AGENT "NodeMCU"
#endif

#ifndef VERSION
#define VERSION "NodeMCU V0.0.2"
#endif

#define DSVCCPIN D5 // Digital pint to use as VCC for the DHT sensor
#define DHTPIN D6     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11

ESP8266WiFiMulti WiFiMulti;
DHT dht(DHTPIN, DHTTYPE);

float gamma(float t, float h);
float dewpoint(float t, float h);
String getReadingPayload();

void setup() {
  //  Serial.begin(115200);
  pinMode(DSVCCPIN, OUTPUT);
  digitalWrite(DSVCCPIN, LOW);
  dht.begin();
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(STASSID, STAPSK);
}

void loop() {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    String jsonData = getReadingPayload();

    if (jsonData.length() > 2) {

      std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
      client->setFingerprint(fingerprint);

      HTTPClient https;

      if (https.begin(*client, APIURI)) {
        https.addHeader("Content-Type", "application/json");
        https.addHeader("User-Agent", USER_AGENT);

        int httpCode = https.POST(jsonData);

        //        // httpCode will be negative on error
        //        if (httpCode > 0) {
        //          String payload = https.getString();
        //          Serial.println(payload);
        //
        //          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        //            Serial.printf("[HTTPS] POST... success with successful status code: %d\n", httpCode);
        //          } else {
        //            Serial.printf("[HTTPS] POST... failed with unsuccessful status code: %d\n", httpCode);
        //          }
        //        } else {
        //          Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
        //        }

        https.end();
      } else {
        //        Serial.printf("[HTTPS] Unable to connect\n");
      }
    }

    //  Serial.println("Wait 10s before next round...");
    //  delay(10000);

    // Deep sleep mode for 15 min, the ESP8266 wakes up by itself when GPIO 16 (D0 in NodeMCU board) is connected to the RESET pin
    ESP.deepSleep(60e6 /* 1 min */ * 15);
    ////ESP.deepSleep(10e6);
  }
}

float gamma(float t, float h) {
  return log(h / 100.0) + (b * t) / (c + t);
}

float dewpoint(float t, float h) {
  float g = gamma(t, h);
  return c * g / (b - g);
}

String getReadingPayload() {
  // Turn on the VCC for the DHT sensor
  digitalWrite(DSVCCPIN, HIGH);

  // Wait a few seconds between measurements.
  delay(2000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();

  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  //  // Read temperature as Fahrenheit (isFahrenheit = true)
  //  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) /*|| isnan(f)*/) {
    //    Serial.println(F("Failed to read from DHT sensor!"));
    return "";
  }

  //  // Compute heat index in Fahrenheit (the default)
  //  float hif = dht.computeHeatIndex(f, h);

  // Compute heat index in Celsius (isFahrenheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  // Compute dew point in Celsius
  float dp = dewpoint(t, h);

  // Turn off the VCC for the DHT sensor
  digitalWrite(DSVCCPIN, LOW);

  return String("{\"sender\":\"" USER_AGENT "\",\"event\":\"readings\",\"readings\":[{\"sensor\":\"DHT11\",\"humidity\":") +
         String(h, 2) +
         String(",\"temperature\":") +
         String(t, 2) +
         String(",\"heatindex\":") +
         String(hic, 2) +
         String(",\"dewpoint\":") +
         String(dp, 2) +
         String("}],\"version\":\"" VERSION "\"}");
}
