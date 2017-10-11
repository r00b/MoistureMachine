#include "SparkFunMicroOLED/SparkFunMicroOLED.h"
#include <SparkFunRHT03.h>
#include <blynk.h>

// soil moisture sensor defs
#define soilPin   A0       // soil moisture sensor data pin
#define soilPower D7       // soil moisture sensor power pin
// #define DELAY     3600000  // 1 hour delay
#define DELAY     5000    // 5 second delay

// temp sensor defs
#define RHT03_DATA_PIN  D3   // RHT03 data pin
// const int LIGHT_PIN = A4;   // photocell analog output
RHT03 rht;

// OLED defs

#define PIN_RESET D6  // RST to pin 6
#define PIN_DC    D5  // DC to pin 5 (required for SPI)
#define PIN_CS    A2  // CS to pin A2 (required for SPI)

MicroOLED oled(MODE_SPI, PIN_RESET, PIN_DC, PIN_CS);

// Blynk defs

char auth[] = "38916c7587c246d287037e389a7fa085";

// Other defs

#define THRESHOLD 50  // saturation warning level

int percentage = 0;
int runs = 0;

String res = "";

void setup() {
  oled.begin();
  Serial.begin(9600); // open serial over USB
  pinMode(soilPower, OUTPUT);   //Set D7 as an power OUTPUT
  digitalWrite(soilPower, LOW); //Set to LOW so no power is flowing through the sensor

  rht.begin(RHT03_DATA_PIN); // init temp sensor
  // pinMode(LIGHT_PIN, INPUT); // Set the photocell pin as an INPUT.

  // set up communication with other Photon
  Particle.subscribe("rob-camila", externalAlert);
  Blynk.begin(auth); // initiate Blynk library
}

void loop() {
  Blynk.run();

  int tempF = readRHT(1);
  int humidity = readRHT(0);
  // int light = readLight();
  int saturation = readSaturation();

  Blynk.virtualWrite(V0, tempF);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, percentage);

  runs++;
  Particle.publish("rob-camila", "hour");
  if (runs <= 4) {
    String runPrefix = String("<string>RUN ") + String(runs) + String(": </strong>");
    String sat = String("Sat ") + String(saturation) + "% / ";
    String temp = String("Temp ") + String(tempF) + String("°F / ");
    String humid = String("Humid ") + String(humidity) + String("%<br>");
    // String lit = String("Light ") + String(light) + String("<br>");

    res = res + runPrefix + sat + temp + humid;
  } else {
    Particle.publish("DATA_REPORT", res);
    res = "";
    runs = 0;
  }
}

float readRHT(int choice) {
  rht.update();
  switch (choice) {
    case 0:
      return rht.humidity();
      break;
    case 1:
      return rht.tempF();
      break;
    case 2:
      return rht.tempC();
      break;
  }
}

// int readLight() {
//   unsigned int light = analogRead(LIGHT_PIN);
//   Serial.print(analogRead(LIGHT_PIN));
//   return light;
// }

int readSaturation() {
  // read the moisture percentage
  // Serial.print("Soil Moisture = ");
  int rawReading = readSoil();
  // Serial.println(rawReading);
  // put it on a calibrated scale
  percentage = scaleReading(rawReading);
// Reading 0: 88% saturation, 75F,
  // trigger low moisture event if necessary
  if (percentage < THRESHOLD) {
    Particle.publish("MOISTURE", "LOW");
  }

  // print to screen
  oled.clear(PAGE);
  oled.setFontType(0);
  renderString(0, 10, "Saturation is");
  oled.setFontType(2);
  renderString(0, 30, String(percentage));
  oled.setFontType(0);
  if (percentage < 100) {
    renderString(25, 30,"%");
  } else {
    renderString(35, 30,"%");
  }
  oled.display();
  delay(DELAY);
  return percentage;
}

void renderString(int x, int y, String string)
{
  oled.setCursor(x, y);
  oled.print(string);
}

int readSoil() {
  digitalWrite(soilPower, HIGH);  // turn on sensor
  delay(10);
  int reading = analogRead(soilPin);      // read the SIG value from sensor
  digitalWrite(soilPower, LOW);   // turn off sensor
  return reading;
}

int scaleReading(int rawReading) {
  int baseline = 2200;
  if (rawReading < baseline) {
    return 0;
  }
  return (int)((rawReading - baseline) * ((float)100 / (float)1100));
}

void externalAlert(const char *event, const char *data) {
  String payload = data;
  if (payload == "Great Day!") {
    oled.clear(PAGE);
    oled.setFontType(0);
    renderString(0, 0, data);
    oled.display();
  }
}
