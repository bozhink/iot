#define ARDUINO 100

#include <Arduino.h>

#include "Adafruit_BMP3XX.h"

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include "_SystemSettings_.h"
#include "_WiFiSettings_.h"
#include "_HttpHeaders_.h"
#include "Settings.h"

#define SEALEVELPRESSURE_HPA (1013.25)

const String sensor = String(IOT_SENSOR);
const String sender = String(IOT_USER_AGENT);
const String event = String(IOT_EVENT);
const String version = String(IOT_VERSION);

ESP8266WiFiMulti WiFiMulti;

Adafruit_BMP3XX bmp; // I2C

bool configureSensor();
String getReading();
String getPayload(String reading);

void setup()
{
    // USE_SERIAL.begin(115200);
    // while (!USE_SERIAL)
    // {
    // }
    // USE_SERIAL.setDebugOutput(true);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    if (!configureSensor())
    {
        digitalWrite(LED_BUILTIN, HIGH);
        while (1)
        {
            // Loop forever
        }
    }

    WiFiMulti.addAP(STASSID, STAPASS);
}

void loop()
{
    // wait for WiFi connection
    if ((WiFiMulti.run() == WL_CONNECTED))
    {
        String reading = getReading();

        if (reading.length() > 2) // The reading must be valid JSON object.
        {
            String payload = getPayload(reading);
            // USE_SERIAL.println(payload);

            HTTPClient http;

            // USE_SERIAL.print("[HTTP] begin...\n");
            // configure target server and url
            //http.begin("https://192.168.1.12/test.html", "7a 9c f4 db 40 d3 62 5a 6e 21 bc 5c cc 66 c8 3e a1 45 59 38"); //HTTPS
            http.begin(IOT_API_URI); //HTTP
            http.addHeader(CONTENT_TYPE_HTTP_HEADER_NAME, CONTENT_TYPE_HTTP_HEADER_VALUE);
            http.addHeader(USER_AGENT_HTTP_HEADER_NAME, IOT_USER_AGENT);
            http.addHeader(IOT_EVENT_HTTP_HEADER_NAME, IOT_EVENT);
            http.addHeader(IOT_VERSION_HTTP_HEADER_NAME, IOT_VERSION);

            // USE_SERIAL.print("[HTTP] POST...\n");
            // start connection and send HTTP header
            int httpCode = http.POST(payload);

            // // httpCode will be negative on error
            // if (httpCode > 0)
            // {
            //     // HTTP header has been send and Server response header has been handled
            //     // USE_SERIAL.printf("[HTTP] POST... code: %d\n", httpCode);

            //     // file found at server
            //     if (httpCode == HTTP_CODE_OK)
            //     {
            //         // USE_SERIAL.println(http.getString());
            //     }
            // }
            // else
            // {
            //     // USE_SERIAL.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
            // }

            http.end();
        }
    }

    delay(30000);
}

bool configureSensor()
{
    if (bmp.begin())
    {
        // Set up oversampling and filter initialization
        bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
        bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
        bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
        //bmp.setOutputDataRate(BMP3_ODR_50_HZ);
        return true;
    }

    return false;
}

String getReading()
{
    if (bmp.performReading())
    {
        const String temperature = String(bmp.temperature, 2);                     /* [*C] */
        const String pressure = String(bmp.pressure / 100.0, 2);                   /* [hPa] */
        const String altitude = String(bmp.readAltitude(SEALEVELPRESSURE_HPA), 2); /* [m] */

        return "{\"sensor\":\"" + sensor + "\",\"temperature\":" + temperature + ",\"pressure\":" + pressure + ",\"altitude\":" + altitude + "}";
    }

    return "";
}

String getPayload(String reading)
{
    return "{\"sender\":\"" + sender + "\",\"event\":\"" + event + "\",\"readings\":[" + reading + "],\"version\":\"" + version + "\"}";
}