#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

#define DHTPIN 15
#define DHTTYPE DHT22

#define HEALTH_CHECK_PERIOD 10000

// Definition of LED pins
#define LED_AZUL 2
#define LED_VERDE 4
#define LED_ROJO 17
#define LED_AMARILLO 16

// Gas sensor pin
#define GAS_SENSOR_PIN 34

struct HealthMonitor
{
	int FAILING_RATE_PERIOD_CYCLES; // period in cycles for relative failing rate
	int FAILURES_SINCE_STARTUP;		// number of failures since startup
	int REMAINING_CYCLES;			// remaining cycles until next check
	int FAILURES_SINCE_LAST_CHECK;	// number of failures since last check
	int REQUESTS_SINCE_LAST_CHECK;	// number of requests since last check
	int REQUESTS_SINCE_STARTUP;		// number of requests since startup
	int FAILING_RATE;				// failing rate in the last period
	void (*reset)(struct HealthMonitor *);
};

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long previousMillis = 0;
unsigned long previousHealthCheckMillis = 0;
const long interval = 250; // Flicker interval in milliseconds
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
	Serial.begin(115200);
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

void printHealthStatus(HealthMonitor *healthMonitor, const char *monitorName)
{
	Serial.print(monitorName);
	Serial.print(" - Failures since startup: ");
	Serial.print(healthMonitor->FAILURES_SINCE_STARTUP);
	Serial.print(", Failures since last check: ");
	Serial.print(healthMonitor->FAILURES_SINCE_LAST_CHECK);
	Serial.print(", Requests since last check: ");
	Serial.print(healthMonitor->REQUESTS_SINCE_LAST_CHECK);
	Serial.print(", Failing rate: ");
	Serial.print(healthMonitor->FAILING_RATE);
	Serial.println("%");
}

void loop()
{
	unsigned long currentMillis = millis();

	if (currentMillis - previousMillis >= interval)
	{
		previousMillis = currentMillis;
		ledAzulState = !ledAzulState;
		digitalWrite(LED_AZUL, ledAzulState);
	}

	// Check if the health check period is completed
	if (currentMillis - previousHealthCheckMillis >= HEALTH_CHECK_PERIOD)
	{
		previousHealthCheckMillis = currentMillis;

		// Print health status for each monitor
		printHealthStatus(&gasHealthMonitor, "Gas Monitor");
		printHealthStatus(&temperatureHealthMonitor, "Temperature Monitor");
		printHealthStatus(&humidityHealthMonitor, "Humidity Monitor");

		// Reset the health monitors for the next period
		gasHealthMonitor.reset(&gasHealthMonitor);
		temperatureHealthMonitor.reset(&temperatureHealthMonitor);
		humidityHealthMonitor.reset(&humidityHealthMonitor);
	}

	// DHT22 sensor reading
	float h = dht.readHumidity();
	humidityHealthMonitor.REQUESTS_SINCE_STARTUP += 1;
	float t = dht.readTemperature();
	temperatureHealthMonitor.REQUESTS_SINCE_STARTUP += 1;
	int gasValue = analogRead(GAS_SENSOR_PIN);
	gasHealthMonitor.REQUESTS_SINCE_STARTUP += 1;
	if (gasValue < 0 || gasValue > 4095)
	{
		gasHealthMonitor.FAILURES_SINCE_STARTUP += 1;
		gasHealthMonitor.FAILURES_SINCE_LAST_CHECK += 1;
	}

	// We check if the readings are valid
	if (isnan(h) || isnan(t))
	{
		if (isnan(h))
		{
			humidityHealthMonitor.FAILURES_SINCE_STARTUP += 1;
			humidityHealthMonitor.FAILURES_SINCE_LAST_CHECK += 1;
		}

		if (isnan(t))
		{
			temperatureHealthMonitor.FAILURES_SINCE_STARTUP += 1;
			temperatureHealthMonitor.FAILURES_SINCE_LAST_CHECK += 1;
		}

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
	Serial.print("Gas: ");
	Serial.println(gasValue);

	// LEDs and LCD display
	int gasThreshold = 2500;

	if (gasValue >= gasThreshold)
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

	delay(500);
}
