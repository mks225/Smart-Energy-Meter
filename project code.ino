#define BLYNK_TEMPLATE_ID "TMPL310ItBqm1"
#define BLYNK_TEMPLATE_NAME "SMART ENERGY METER"
#define BLYNK_AUTH_TOKEN "6YEI2SW9EkFwMhRWFqQ-d0_Nl0vyjrXa"
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Wifi.h>
#include <WifiClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ZMPT101B.h"
#include "ACS712.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

char auth[] = "6YEI2SW9EkFwMhRWFqQ-d0_Nl0vyjrXa";
BlynkTimer timer;

ZMPT101B voltageSensor(34);
ACS712 currentSensor(ACS712_20A, 36);

float P = 0;
float U = 0;
float I = 0;
long dt = 0;
float CulmPwh = 0;
float units = 0;
long changeScreen = 0;
float lastSample = 0;

unsigned long lasttime = 0;
long ScreenSelect = 0;

void setup() {
  Serial.begin(9600);

  Blynk.begin(auth, "Anish", "anish098");

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) {}
  }

  // Calibration commands need to be run on first upload
  // CalibCurrent();
  // CalibVoltage();

  timer.setInterval(1000L, sendSensorDataToBlynk); // Update Blynk every second
}

void loop() {
  Blynk.run();
  timer.run();

  U = voltageSensor.getRmsVoltage();
  if (U < 55) {
    U = 0;
    CulmPwh = 0;
  }

  I = currentSensor.getCurrentAC();
  dt = micros() - lastSample;

  if (I < 0.15) {
    I = 0;
    CulmPwh = 0;
  }

  P = U * I;
  CulmPwh = CulmPwh + P * (dt / 3600); // Accumulate power consumption
  units = CulmPwh / 1000;

  if (millis() - changeScreen > 5000) {
    ScreenSelect += 1;
    changeScreen = millis();
  }

  if (millis() - lasttime > 500) {
    switch (ScreenSelect % 4) {
      case 0:
        displayVoltCurrent();
        break;
      case 1:
        displayInstPower();
        break;
      case 2:
        displayEnergy();
        break;
      case 3:
        displayUnits();
        break;
    }
  }
  lastSample = micros();
}

void displayVoltCurrent() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(3);
  displayCenter(String(U) + "V", 3);
  display.setTextSize(3);
  displayCenter(String(I) + "A", 33);
  display.display();
  lasttime = millis();
}

void displayInstPower() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  displayCenter("Power", 3);
  display.setTextSize(3);
  if (P > 1000) {
    displayCenter(String(P / 1000) + "kW", 30);
  } else {
    displayCenter(String(P) + "W", 30);
  }
  display.display();
  lasttime = millis();
}

void displayEnergy() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  if (CulmPwh > 1000000000) {
    display.setTextSize(2);
    displayCenter("Energy kWh", 3);
    display.setTextSize(3);
    displayCenter(String(CulmPwh / 1000000000), 30);
  } else if (CulmPwh < 1000000000 && CulmPwh > 1000000) {
    display.setTextSize(2);
    displayCenter("Energy Wh", 3);
    display.setTextSize(3);
    displayCenter(String(CulmPwh / 1000000), 30);
  } else if (CulmPwh < 1000000 && CulmPwh > 1000) {
    display.setTextSize(2);
    displayCenter("Energy mWh", 3);
    display.setTextSize(3);
    displayCenter(String(CulmPwh / 1000), 30);
  } else {
    display.setTextSize(2);
    displayCenter("Energy uWh", 3);
    display.setTextSize(3);
    displayCenter(String(CulmPwh), 30);
  }
  display.display();
  lasttime = millis();
}

void displayUnits() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  if (units > 1000000) {
    display.setTextSize(2);
    displayCenter("Units", 3);
    display.setTextSize(3);
    displayCenter(String(units / 1000000), 30);
  } else if (units < 1000000 && units > 1000) {
    display.setTextSize(2);
    displayCenter("MilliUnits", 3);
    display.setTextSize(3);
    displayCenter(String(units / 1000), 30);
  } else {
    display.setTextSize(2);
    displayCenter("MicroUnits", 3);
    display.setTextSize(3);
    displayCenter(String(units), 30);
  }
  display.display();
  lasttime = millis();
}

void sendSensorDataToBlynk() {
  Blynk.virtualWrite(V1, U);      // Send voltage to V1 in Blynk app
  Blynk.virtualWrite(V2, I);      // Send current to V2 in Blynk app
  Blynk.virtualWrite(V3, P);      // Send power to V3 in Blynk app
  Blynk.virtualWrite(V4, CulmPwh); // Send cumulative power to V4 in Blynk app
  Blynk.virtualWrite(V5, units);   // Send units to V5 in Blynk app
}

void displayCenter(String text, int line) {
  int16_t x1, y1;
  uint16_t width, height;
  display.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);
  display.setCursor((SCREEN_WIDTH - width) / 2, line);
  display.println(text);
  display.display();
}
