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

ESP8266WiFiMulti WiFiMulti;

Adafruit_BMP3XX bmp; // I2C

bool configureSensor();

void setup()
{
    USE_SERIAL.begin(115200);
    while (!USE_SERIAL)
    {
    }
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
        if (!bmp.performReading())
        {
            USE_SERIAL.println("Failed to perform reading :(");
            return;
        }
        USE_SERIAL.print("Temperature = ");
        USE_SERIAL.print(bmp.temperature);
        USE_SERIAL.println(" *C");

        USE_SERIAL.print("Pressure = ");
        USE_SERIAL.print(bmp.pressure / 100.0);
        USE_SERIAL.println(" hPa");

        USE_SERIAL.print("Approx. Altitude = ");
        USE_SERIAL.print(bmp.readAltitude(SEALEVELPRESSURE_HPA));
        USE_SERIAL.println(" m");

        USE_SERIAL.println();
        delay(2000);

        HTTPClient http;

        USE_SERIAL.print("[HTTP] begin...\n");
        // configure target server and url
        //http.begin("https://192.168.1.12/test.html", "7a 9c f4 db 40 d3 62 5a 6e 21 bc 5c cc 66 c8 3e a1 45 59 38"); //HTTPS
        http.begin(API_URI); //HTTP
        http.addHeader(CONTENT_TYPE_HTTP_HEADER_NAME, CONTENT_TYPE_HTTP_HEADER_VALUE);
        http.addHeader(USER_AGENT_HTTP_HEADER_NAME, USER_AGENT);
        http.addHeader(IOT_EVENT_HTTP_HEADER_NAME, EVENT);
        http.addHeader(IOT_VERSION_HTTP_HEADER_NAME, VERSION);

        USE_SERIAL.print("[HTTP] POST...\n");
        // start connection and send HTTP header
        int httpCode = http.POST("{}");

        // httpCode will be negative on error
        if (httpCode > 0)
        {
            // HTTP header has been send and Server response header has been handled
            USE_SERIAL.printf("[HTTP] POST... code: %d\n", httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK)
            {
                String payload = http.getString();
                USE_SERIAL.println(payload);
            }
        }
        else
        {
            USE_SERIAL.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
    }

    delay(10000);
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