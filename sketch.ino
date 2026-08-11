/*
  Smart Classroom Automation - ESP32

  Reads temperature and humidity (DHT22), ambient light (LDR) and occupancy
  (PIR), then drives the classroom lights, the fan relay and an over
  temperature buzzer. Current state is shown on a 128x64 SSD1306 OLED and
  published to a ThingSpeak channel over WiFi.
*/

#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ThingSpeak.h>

#include "config.h"
#include "secrets.h"

DHT dht(PIN_DHT, DHT22);
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
WiFiClient net;

// latest sensor snapshot
float tempC = NAN;
float humidity = NAN;
float lightPct = 0.0f;
bool motion = false;
bool occupied = false;

// actuator state
bool lightsOn = false;
bool fanOn = false;
bool alarmOn = false;
bool beepHigh = false;

unsigned long lastSensorMs = 0;
unsigned long lastDisplayMs = 0;
unsigned long lastBeepMs = 0;
unsigned long lastMotionMs = 0;
unsigned long lastUploadMs = 0;
unsigned long lastWifiTryMs = 0;

int lastUploadStatus = 0;   // 200 once a ThingSpeak write has succeeded

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("\nSmart Classroom Automation booting"));

  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_LIGHT, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_LIGHT, LOW);
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  analogReadResolution(12);          // 0..4095
  analogSetPinAttenuation(PIN_LDR, ADC_11db);   // full 0..3.3 V swing

  dht.begin();

  Wire.begin(PIN_SDA, PIN_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 not found, continuing without display"));
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 24);
    display.println(F("Smart Classroom"));
    display.println(F("starting up..."));
    display.display();
  }

  connectWifi();
  ThingSpeak.begin(net);

  lastMotionMs = millis();
}

// Blocks for at most 10 s so a dead network never stops the automation.
void connectWifi() {
  Serial.printf("WiFi: joining %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000UL) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("WiFi: connected, IP "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("WiFi: not connected, running offline"));
  }
  lastWifiTryMs = millis();
}

void loop() {
  unsigned long now = millis();

  readMotion(now);

  if (now - lastSensorMs >= SENSOR_PERIOD_MS) {
    lastSensorMs = now;
    readEnvironment();
    applyRules(now);
    logSerial();
  }

  driveBuzzer(now);

  if (now - lastDisplayMs >= DISPLAY_PERIOD_MS) {
    lastDisplayMs = now;
    drawDisplay();
  }

  if (now - lastUploadMs >= TS_PERIOD_MS) {
    lastUploadMs = now;
    uploadThingSpeak();
  }
}

// ---------------------------------------------------------------- telemetry

void uploadThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiTryMs >= 15000UL) connectWifi();
    return;
  }
  if (TS_CHANNEL_ID == 0UL) {
    Serial.println(F("ThingSpeak: channel not configured, skipping upload"));
    return;
  }

  ThingSpeak.setField(1, tempC);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, lightPct);
  ThingSpeak.setField(4, (int)occupied);
  ThingSpeak.setField(5, (int)lightsOn);
  ThingSpeak.setField(6, (int)fanOn);
  ThingSpeak.setField(7, (int)alarmOn);
  ThingSpeak.setStatus(occupied ? "occupied" : "empty");

  lastUploadStatus = ThingSpeak.writeFields(TS_CHANNEL_ID, TS_WRITE_API_KEY);
  if (lastUploadStatus == 200) {
    Serial.println(F("ThingSpeak: update accepted"));
  } else {
    Serial.printf("ThingSpeak: write failed, HTTP %d\n", lastUploadStatus);
  }
}

// ------------------------------------------------------------------ input

// PIR is sampled every loop so short pulses are never missed.
void readMotion(unsigned long now) {
  motion = digitalRead(PIN_PIR) == HIGH;
  if (motion) {
    lastMotionMs = now;
  }
  occupied = (now - lastMotionMs) < VACANCY_MS;
}

void readEnvironment() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) tempC = t;
  if (!isnan(h)) humidity = h;
  if (isnan(t) || isnan(h)) {
    Serial.println(F("DHT22 read failed, keeping last good values"));
  }

  int raw = analogRead(PIN_LDR);
  lightPct = 100.0f * (float)(raw - LDR_RAW_DARK) /
             (float)(LDR_RAW_BRIGHT - LDR_RAW_DARK);
  lightPct = constrain(lightPct, 0.0f, 100.0f);
}

// -------------------------------------------------------------- decisions

void applyRules(unsigned long now) {
  // Lights: only while the room is occupied, and only when it is dark.
  // Hysteresis keeps the LED from chattering around the threshold.
  if (!occupied) {
    lightsOn = false;
  } else if (!lightsOn && lightPct < DARK_PCT) {
    lightsOn = true;
  } else if (lightsOn && lightPct > DARK_PCT + LIGHT_HYST_PCT) {
    lightsOn = false;
  }

  // Fan: occupied and warm.
  if (!occupied) {
    fanOn = false;
  } else if (!isnan(tempC)) {
    if (!fanOn && tempC > FAN_ON_C) {
      fanOn = true;
    } else if (fanOn && tempC < FAN_ON_C - FAN_HYST_C) {
      fanOn = false;
    }
  }

  // Alarm: temperature emergency, regardless of occupancy.
  alarmOn = !isnan(tempC) && tempC > ALARM_C;

  digitalWrite(PIN_LIGHT, lightsOn ? HIGH : LOW);
  digitalWrite(PIN_RELAY, fanOn ? HIGH : LOW);

  if (!alarmOn) {
    digitalWrite(PIN_BUZZER, LOW);
    beepHigh = false;
    lastBeepMs = now;
  }
}

// ----------------------------------------------------------------- output

void driveBuzzer(unsigned long now) {
  if (!alarmOn) return;
  if (now - lastBeepMs >= BEEP_PERIOD_MS) {
    lastBeepMs = now;
    beepHigh = !beepHigh;
    digitalWrite(PIN_BUZZER, beepHigh ? HIGH : LOW);
  }
}

void drawDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("SMART CLASSROOM"));
  display.setCursor(104, 0);
  if (WiFi.status() != WL_CONNECTED) {
    display.print(F("OFF"));
  } else {
    display.print(lastUploadStatus == 200 ? F("TS ") : F("NET"));
  }
  display.drawFastHLine(0, 10, OLED_WIDTH, SSD1306_WHITE);

  display.setCursor(0, 16);
  display.print(F("Temp  "));
  if (isnan(tempC)) display.print(F("--")); else display.print(tempC, 1);
  display.print(F(" C"));

  display.setCursor(0, 26);
  display.print(F("Hum   "));
  if (isnan(humidity)) display.print(F("--")); else display.print(humidity, 0);
  display.print(F(" %"));

  display.setCursor(0, 36);
  display.print(F("Light "));
  display.print(lightPct, 0);
  display.print(F(" %"));

  display.setCursor(0, 46);
  display.print(F("Room  "));
  display.print(occupied ? F("OCCUPIED") : F("EMPTY"));

  display.drawFastHLine(0, 55, OLED_WIDTH, SSD1306_WHITE);
  display.setCursor(0, 57);
  display.print(F("LGT:"));
  display.print(lightsOn ? F("ON ") : F("OFF"));
  display.print(F(" FAN:"));
  display.print(fanOn ? F("ON ") : F("OFF"));
  if (alarmOn) display.print(F(" !"));

  display.display();
}

void logSerial() {
  Serial.printf("T=%.1fC H=%.0f%% L=%.0f%% motion=%d occupied=%d "
                "lights=%d fan=%d alarm=%d\n",
                tempC, humidity, lightPct, motion, occupied,
                lightsOn, fanOn, alarmOn);
}
