#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#define DHTPIN 15
#define DHTTYPE DHT22

#define HEALTH_CHECK_PERIOD 10000

// Definition of LED pins
#define LED_AZUL 2
#define LED_VERDE 4
#define LED_ROJO 17
#define LED_AMARILLO 16

// Gas sensor pins
#define GAS_OXYGEN_PIN 34
#define GAS_CO2_PIN 35
#define GAS_ETHYLENE_PIN 32
#define GAS_AMMONIA_PIN 33
#define GAS_SO2_PIN 35

// WiFi Credentials
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""

// Fake Edge API URL
#define ENDPOINT_URL "https://iot-edge-sample-20211d744.free.beeceptor.com/api/v1/data-records"

// HTTP Client
HTTPClient httpClient;

// HTTP Header Parameters
#define CONTENT_TYPE_HEADER "Content-Type"
#define APPLICATION_JSON "application/json"

#define DEVICE_ID "HC001"

struct HealthMonitor
{
    int FAILING_RATE_PERIOD_CYCLES; // period in cycles for relative failing rate
    int FAILURES_SINCE_STARTUP;     // number of failures since startup
    int REMAINING_CYCLES;           // remaining cycles until next check
    int FAILURES_SINCE_LAST_CHECK;  // number of failures since last check
    int REQUESTS_SINCE_LAST_CHECK;  // number of requests since last check
    int REQUESTS_SINCE_STARTUP;     // number of requests since startup
    int FAILING_RATE;               // failing rate in the last period
    void (*reset)(struct HealthMonitor *);
};

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long previousMillis = 0;
const long interval = 60000;  // Interval of 1 minute
bool ledAzulState = false;
HealthMonitor gasHealthMonitor;
HealthMonitor temperatureHealthMonitor;
HealthMonitor humidityHealthMonitor;

void HealthMonitor_reset(struct HealthMonitor *healthMonitor)
{
    healthMonitor->REMAINING_CYCLES = healthMonitor->FAILING_RATE_PERIOD_CYCLES;
    healthMonitor->FAILURES_SINCE_LAST_CHECK = 0;
    healthMonitor->REQUESTS_SINCE_LAST_CHECK = 0;
}

void initializeHealthMonitor(HealthMonitor *healthMonitor, int periodCycles)
{
    healthMonitor->FAILING_RATE_PERIOD_CYCLES = periodCycles;
    healthMonitor->FAILURES_SINCE_STARTUP = 0;
    healthMonitor->REMAINING_CYCLES = periodCycles;
    healthMonitor->FAILURES_SINCE_LAST_CHECK = 0;
    healthMonitor->REQUESTS_SINCE_LAST_CHECK = 0;
    healthMonitor->FAILING_RATE = 0;
    healthMonitor->reset = HealthMonitor_reset;
}

void setup()
{
    // Serial Output initialization
    Serial.begin(115200);

    // WiFi Setup
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println();
    Serial.print("connected: ");
    Serial.println(WiFi.localIP());

    dht.begin();
    lcd.init();
    lcd.backlight();

    // Configuration of the LED pins as outputs
    pinMode(LED_AZUL, OUTPUT);
    pinMode(LED_VERDE, OUTPUT);
    pinMode(LED_ROJO, OUTPUT);
    pinMode(LED_AMARILLO, OUTPUT);

    // LEDs are off at startup
    digitalWrite(LED_AZUL, LOW);
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_ROJO, LOW);
    digitalWrite(LED_AMARILLO, LOW);

    // Welcome messages on the LCD display
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Welcome");
    delay(2000);

    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("DittoBox");
    delay(2000);

    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Starting");
    delay(2000);

    lcd.clear();

    // Initialize health monitors
    initializeHealthMonitor(&gasHealthMonitor, HEALTH_CHECK_PERIOD);
    initializeHealthMonitor(&temperatureHealthMonitor, HEALTH_CHECK_PERIOD);
    initializeHealthMonitor(&humidityHealthMonitor, HEALTH_CHECK_PERIOD);
}

void loop()
{
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;
        ledAzulState = !ledAzulState;
        digitalWrite(LED_AZUL, ledAzulState);

        // DHT22 sensor reading
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        int gasOxygen = analogRead(GAS_OXYGEN_PIN);
        int gasCO2 = analogRead(GAS_CO2_PIN);
        int gasEthylene = analogRead(GAS_ETHYLENE_PIN);
        int gasAmmonia = analogRead(GAS_AMMONIA_PIN);
        int gasSO2 = analogRead(GAS_SO2_PIN);

        // Convert gas sensor readings to percentage (0-100%)
        float gasOxygenPercent = (gasOxygen / 4095.0) * 100;
        float gasCO2Percent = (gasCO2 / 4095.0) * 100;
        float gasEthylenePercent = (gasEthylene / 4095.0) * 100;
        float gasAmmoniaPercent = (gasAmmonia / 4095.0) * 100;
        float gasSO2Percent = (gasSO2 / 4095.0) * 100;

        // We check if the readings are valid
        if (isnan(h) || isnan(t))
        {
            Serial.println("Error al leer del sensor DHT!");
            digitalWrite(LED_ROJO, HIGH);
            digitalWrite(LED_VERDE, LOW);
            digitalWrite(LED_AMARILLO, LOW);
            return;
        }

        // Display on the serial monitor
        Serial.print("Temperatura: ");
        Serial.print(t);
        Serial.print(" °C ");
        Serial.print("Humedad: ");
        Serial.print(h);
        Serial.print(" % ");
        Serial.print("Oxygen: ");
        Serial.print(gasOxygenPercent);
        Serial.print(" % CO2: ");
        Serial.print(gasCO2Percent);
        Serial.print(" % Ethylene: ");
        Serial.print(gasEthylenePercent);
        Serial.print(" % Ammonia: ");
        Serial.print(gasAmmoniaPercent);
        Serial.print(" % SO2: ");
        Serial.println(gasSO2Percent);

        // LEDs and LCD display
        int gasThreshold = 60; // Adjusted threshold for percentage

        if (gasOxygenPercent >= gasThreshold || gasCO2Percent >= gasThreshold || gasEthylenePercent >= gasThreshold || gasAmmoniaPercent >= gasThreshold || gasSO2Percent >= gasThreshold)
        {
            // The input is in poor condition
            digitalWrite(LED_AMARILLO, HIGH);
            digitalWrite(LED_VERDE, LOW);
            digitalWrite(LED_ROJO, LOW);

            // Display alert message on LCD
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Manzana en mal");
            lcd.setCursor(0, 1);
            lcd.print("estado!");
        }
        else
        {
            // The input is in good condition
            digitalWrite(LED_VERDE, HIGH);
            digitalWrite(LED_AMARILLO, LOW);
            digitalWrite(LED_ROJO, LOW);

            // Display normal data on the LCD
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Manzana");
            lcd.setCursor(0, 1);
            lcd.print("T:");
            lcd.print((int)t);
            lcd.print((char)223);
            lcd.print("C H:");
            lcd.print((int)h);
            lcd.print("%");
        }

        // Send data to REST API
        StaticJsonDocument<200> dataRecord;
        dataRecord["deviceId"] = DEVICE_ID;
        dataRecord["temperature"] = t;
        dataRecord["humidity"] = h;
        dataRecord["gasOxygen"] = gasOxygenPercent;
        dataRecord["gasCO2"] = gasCO2Percent;
        dataRecord["gasEthylene"] = gasEthylenePercent;
        dataRecord["gasAmmonia"] = gasAmmoniaPercent;
        dataRecord["gasSO2"] = gasSO2Percent;

        String dataRecordResource;
        serializeJson(dataRecord, dataRecordResource);
        httpClient.begin(ENDPOINT_URL);
        httpClient.addHeader(CONTENT_TYPE_HEADER, APPLICATION_JSON);

        // Make HTTP POST Request
        int httpResponseCode = httpClient.POST(dataRecordResource);

        // Check Response
        if (httpResponseCode > 0)
        {
            String responseResource = httpClient.getString();
            StaticJsonDocument<200> response;
            deserializeJson(response, responseResource);
            serializeJsonPretty(response, Serial);
        }
        else
        {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        }

        // Close Request session
        httpClient.end();
    }
}
