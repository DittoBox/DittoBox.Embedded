#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

#define DHTPIN 15
#define DHTTYPE DHT22

// Definition of LED pins
#define LED_AZUL 2
#define LED_VERDE 4
#define LED_ROJO 17
#define LED_AMARILLO 16

// Gas sensor pin
#define GAS_SENSOR_PIN 34

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long previousMillis = 0;
const long interval = 250;  // Flicker interval in milliseconds
bool ledAzulState = false;

void setup() {
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
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    ledAzulState = !ledAzulState;
    digitalWrite(LED_AZUL, ledAzulState);
  }

  // DHT22 sensor reading
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int gasValue = analogRead(GAS_SENSOR_PIN);

  // We check if the readings are valid
  if (isnan(h) || isnan(t)) {
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

  if (gasValue >= gasThreshold) {
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
  } else {
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
