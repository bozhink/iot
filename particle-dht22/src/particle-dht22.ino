/*
 * Project particle-dht22
 * Description: DHT22 sampling
 * Author: Bozhin Karaivanov
 * Date: 2019-11-12
 */

#include <math.h>
#include "Adafruit_DHT_Particle.h"

// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain

#define MINIMAL_TIME_TO_SLEEP 300
#define MAXIMAL_SYSTEM_CHECKS 10

#define NUMBER_OF_SAMPLES 3

#define POWERPIN D7   // What pin is the power
#define DHTPIN D6     // what pin we're connected to
#define DHTTYPE DHT22 // DHT 22 (AM2302)

// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

int readData(String command);
int setDelay(String command);
void updateVariables(float h, float t, float f, float k, float dp, float hi);
void publishData(float h, float t, float f, float k, float dp, float hi);
void logData(float h, float t, float f, float k, float dp, float hi);
void deviceNameHandler(const char *topic, const char *data);

double tempCelsius = 0;
double tempFahrenheit = 0;
double tempKelvin = 0;
double humidity = 0;
double heatIndex = 0;
double dewPoint = 0;

DHT dht(DHTPIN, DHTTYPE);
int loopCount;
int checkLoopCount;
int delaySeconds;

String deviceName;
String version = String("V0.0.7");

TCPClient client;
byte server[] = {192, 168, 5, 87};
int serverPort = 60135;

bool serialAvailable;

void setup()
{
    Serial.begin(9600);

    Particle.publish("state", "DHT22 start. " + version, PRIVATE);

    // get the name of the device
    Particle.subscribe("particle/device/name", deviceNameHandler, MY_DEVICES);
    Particle.publish("particle/device/name", PRIVATE);
    delay(1000);

    if (Serial.available())
    {
        Serial.println(deviceName + "/DHT22 start. " + version);
    }

    Particle.variable("tempCelsius", tempCelsius);
    Particle.variable("tempFahrenheit", tempFahrenheit);
    Particle.variable("tempKelvin", tempKelvin);
    Particle.variable("humidity", humidity);
    Particle.variable("heatIndex", heatIndex);
    Particle.variable("dewPoint", dewPoint);
    Particle.variable("delaySeconds", delaySeconds);

    Particle.function("readData", readData);
    Particle.function("setDelay", setDelay);

    pinMode(POWERPIN, OUTPUT);
    dht.begin();
    loopCount = 0;
    checkLoopCount = 0;
    delaySeconds = 15 * 60;
    delay(1000);
}

void loop()
{
    checkLoopCount++;
    if (WiFi.ready())
    {
        checkLoopCount = 0;

        if (deviceName != NULL)
        {
            serialAvailable = Serial.available();

            digitalWrite(POWERPIN, HIGH);
            delay(2000);

            readData("loop");
            loopCount++;

            if (loopCount >= NUMBER_OF_SAMPLES)
            {
                loopCount = 0;
                Particle.publish("state", "Going to sleep for several minutes", PRIVATE);
                digitalWrite(POWERPIN, LOW);
                delay(2000);
                System.sleep(SLEEP_MODE_DEEP, max(MINIMAL_TIME_TO_SLEEP, abs(delaySeconds)));
            }
        }
        else
        {
            delay(1000);
        }
    }
    else
    {

        if (checkLoopCount > MAXIMAL_SYSTEM_CHECKS)
        {
            checkLoopCount = 0;
            System.sleep(SLEEP_MODE_DEEP, max(MINIMAL_TIME_TO_SLEEP, abs(delaySeconds)));
        }

        delay(checkLoopCount * 1000);
    }
}

void deviceNameHandler(const char *topic, const char *data)
{
    deviceName = String(data);
}

int setDelay(String command)
{
    if (command == NULL)
    {
        return 1;
    }

    long delay = command.trim().toInt();
    if (delay < 1)
    {
        return 2;
    }

    delaySeconds = (int)(max(10, abs(delay)) % 100000);

    return 0;
}

int readData(String command)
{
    // Wait a few seconds between measurements.
    delay(2000);

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.getHumidity();
    // Read temperature as Celsius
    float t = dht.getTempCelsius();
    // Read temperature as Fahrenheit
    float f = dht.getTempFahrenheit();

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f))
    {
        if (serialAvailable)
        {
            Serial.println("Failed to read from DHT sensor!");
        }

        return 1;
    }

    // Compute heat index
    // Must send in temp in Fahrenheit!
    float hi = dht.getHeatIndex();
    float dp = dht.getDewPoint();
    float k = dht.getTempKelvin();

    updateVariables(h, t, f, k, dp, hi);
    publishData(h, t, f, k, dp, hi);
    logData(h, t, f, k, dp, hi);

    return 0;
}

void updateVariables(float h, float t, float f, float k, float dp, float hi)
{
    humidity = h;
    tempCelsius = t;
    tempFahrenheit = f;
    tempKelvin = k;
    heatIndex = hi;
    dewPoint = dp;
}

void logData(float h, float t, float f, float k, float dp, float hi)
{
    if (serialAvailable)
    {
        // Log data to serial
        Serial.print("Humid: ");
        Serial.print(h);
        Serial.print("% - ");
        Serial.print("Temp: ");
        Serial.print(t);
        Serial.print("*C ");
        Serial.print(f);
        Serial.print("*F ");
        Serial.print(k);
        Serial.print("*K - ");
        Serial.print("DewP: ");
        Serial.print(dp);
        Serial.print("*C - ");
        Serial.print("HeatI: ");
        Serial.print(hi);
        Serial.println("*C");
        Serial.println(Time.timeStr());
    }
}

void publishData(float h, float t, float f, float k, float dp, float hi)
{
    if (client.connected() || client.connect(server, serverPort))
    {
        if (serialAvailable)
        {
            Serial.println("connected");
        }

        String jsonData = String::format(
            "{\"sender\":\"%s\",\"event\":\"%s\",\"readings\":[{\"sensor\":\"%s\",\"humidity\":%4.2f,\"temperature\":%4.2f,\"heatindex\":%4.2f,\"dewpoint\":%4.2f}],\"version\":\"%s\"}",
            deviceName.c_str(), "readings",
            "DHT22", h, t, hi, dp,
            version.c_str());

        client.println("POST /api/v1/events HTTP/1.0");
        client.println(String::format("Host: %d.%d.%d.%d", server[0], server[1], server[2], server[3]));
        client.println(String::format("User-Agent: %s", deviceName));
        client.println("Content-Type: application/json");
        client.println(String::format("Content-Length: %d", jsonData.length()));
        client.println();
        client.println(jsonData);

        client.flush();
        client.stop();

        delay(1000);
    }
    else
    {
        if (serialAvailable)
        {
            Serial.println("connection failed");
        }
    }

    // Publish data to Particle cloud
    String readingsJson = String::format("{\"Hum(\%)\": %4.2f, \"Temp(°C)\": %4.2f, \"DP(°C)\": %4.2f, \"HI(°C)\": %4.2f}", h, t, dp, hi);
    Particle.publish("readings", readingsJson, PRIVATE);
}
