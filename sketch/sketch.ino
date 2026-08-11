/*
  Smart Classroom Automation - ESP32

  Reads temperature and humidity (DHT22), ambient light (LDR) and occupancy
  (PIR), then drives the classroom lights, the fan relay and an over
  temperature buzzer. Current state is shown on a 128x64 SSD1306 OLED and
  published to a ThingSpeak channel over WiFi.
*/

#include "secrets.h"          // must precede BlynkSimpleEsp32.h, which reads
#define BLYNK_PRINT Serial    // BLYNK_TEMPLATE_ID and friends at include time

#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <ThingSpeak.h>

#include "config.h"

DHT dht(PIN_DHT, DHT22);
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
WiFiClient net;      // ThingSpeak channel writes
WiFiClient tbNet;    // TalkBack command polling, kept separate on purpose

// latest sensor snapshot
float tempC = NAN;
float humidity = NAN;
float lightPct = 0.0f;
int ldrRaw = 0;              // raw ADC counts, logged so the map can be calibrated
bool motion = false;
bool occupied = false;

// actuator state
bool lightsOn = false;
bool fanOn = false;            // relay energised, the fan circuit is live
int fanSpeedPct = 0;           // 0..100, PWM duty behind the relay contacts
bool alarmOn = false;
bool beepHigh = false;

unsigned long lastSensorMs = 0;
unsigned long lastDisplayMs = 0;
unsigned long lastBeepMs = 0;
unsigned long lastMotionMs = 0;
unsigned long lastUploadMs = 0;
unsigned long lastWifiTryMs = 0;

int lastUploadStatus = 0;   // 200 once a ThingSpeak write has succeeded

// schedule state
bool timeValid = false;         // NTP has answered at least once
bool inSession = true;          // inside teaching hours, fails open when offline
char clockText[6] = "--:--";

// remote control state. An override beats both the sensors and the timetable,
// which is the whole point of a manual override, and expires by itself.
enum Override { OV_AUTO = 0, OV_FORCE_ON = 1, OV_FORCE_OFF = 2 };
Override fanOverride = OV_AUTO;
Override lightOverride = OV_AUTO;
unsigned long fanOverrideMs = 0;
unsigned long lightOverrideMs = 0;
unsigned long lastTalkbackMs = 0;
unsigned long lastBlynkPushMs = 0;

// The fan setpoint is a live value, not a constant, because the Blynk slider
// writes it. FAN_ON_C is only the power on default. The ramp keeps its width,
// so moving the start temperature moves full speed with it.
float fanOnC = FAN_ON_C;
float fanFullC = FAN_FULL_C;

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

  // Speed output. The relay contacts sit between this pin and the fan, so the
  // duty cycle only reaches the load once the isolation stage is closed.
  if (!ledcAttach(PIN_FAN_PWM, FAN_PWM_FREQ, FAN_PWM_BITS)) {
    Serial.println(F("LEDC attach failed, fan will not vary speed"));
  }
  ledcWrite(PIN_FAN_PWM, 0);

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
  syncClock();
  ThingSpeak.begin(net);
  connectBlynk();

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

// Bounded connect, so a Blynk outage delays startup by 10 seconds at worst
// instead of blocking the classroom forever. Blynk.run() reconnects later.
void connectBlynk() {
  if (strlen(BLYNK_AUTH_TOKEN) == 0 || WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Blynk: disabled or offline, running without it"));
    return;
  }
  Blynk.config(BLYNK_AUTH_TOKEN);
  if (Blynk.connect(10000)) {
    Serial.println(F("Blynk: ready"));
  } else {
    Serial.println(F("Blynk: no connection, will retry in the background"));
  }
}

// Asks NTP for the wall clock. Called once after the network is up; the
// schedule keeps working off the ESP32's own clock afterwards.
void syncClock() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Clock: offline, schedule disabled and automation left on"));
    return;
  }

  configTime(TZ_OFFSET_SEC, TZ_DST_SEC, NTP_SERVER_A, NTP_SERVER_B);

  struct tm t;
  if (getLocalTime(&t, 8000)) {
    timeValid = true;
    Serial.printf("Clock: %04d-%02d-%02d %02d:%02d:%02d local\r\n",
                  t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                  t.tm_hour, t.tm_min, t.tm_sec);
  } else {
    Serial.println(F("Clock: NTP timeout, schedule disabled"));
  }
}

// Decides whether the room is inside teaching hours. With no valid clock this
// deliberately fails open, an unsynced device should still run the classroom.
void updateSchedule() {
  if (!timeValid) {
    inSession = true;
    return;
  }

  struct tm t;
  if (!getLocalTime(&t, 50)) {
    inSession = true;
    return;
  }

  snprintf(clockText, sizeof(clockText), "%02d:%02d", t.tm_hour, t.tm_min);

  int minutes = t.tm_hour * 60 + t.tm_min;
  bool teachingDay = (SCHOOL_DAY_MASK >> t.tm_wday) & 1;

  inSession = teachingDay &&
              minutes >= SCHOOL_START_MIN &&
              minutes < SCHOOL_END_MIN;
}

void loop() {
  unsigned long now = millis();

  // Pumps the Blynk connection: delivers widget writes, answers heartbeats,
  // reconnects if the link dropped. Must be called often, hence before the
  // periodic work rather than inside it.
  Blynk.run();

  readMotion(now);

  if (now - lastSensorMs >= SENSOR_PERIOD_MS) {
    lastSensorMs = now;
    updateSchedule();
    readEnvironment();
    applyRules(now);
    logSerial();
  }

  driveBuzzer(now);

  if (now - lastDisplayMs >= DISPLAY_PERIOD_MS) {
    lastDisplayMs = now;
    drawDisplay();
  }

  if (now - lastBlynkPushMs >= BLYNK_PUSH_MS) {
    lastBlynkPushMs = now;
    pushBlynk();
  }

  if (now - lastTalkbackMs >= TALKBACK_PERIOD_MS) {
    lastTalkbackMs = now;
    pollTalkBack();
  }

  if (now - lastUploadMs >= TS_PERIOD_MS) {
    lastUploadMs = now;
    uploadThingSpeak();
  }
}

// ------------------------------------------------------------------- blynk

// Widget writes land here the moment the user touches the app, with no polling
// interval in between. They set the same override slots TalkBack writes, so the
// two remote paths cannot disagree, whichever command arrived last wins.

BLYNK_WRITE(VP_FAN_MODE) {
  int mode = param.asInt();
  fanOverride = (Override)constrain(mode, 0, 2);
  fanOverrideMs = millis();
  Serial.printf("Blynk: fan mode %d\r\n", mode);
}

BLYNK_WRITE(VP_LIGHT_MODE) {
  int mode = param.asInt();
  lightOverride = (Override)constrain(mode, 0, 2);
  lightOverrideMs = millis();
  Serial.printf("Blynk: lights mode %d\r\n", mode);
}

BLYNK_WRITE(VP_SETPOINT) {
  float requested = param.asFloat();
  float span = fanFullC - fanOnC;          // preserve the ramp width
  fanOnC = constrain(requested, SETPOINT_MIN_C, SETPOINT_MAX_C);
  fanFullC = fanOnC + span;
  Serial.printf("Blynk: fan setpoint now %.1fC, full speed at %.1fC\r\n",
                fanOnC, fanFullC);
}

// Called on connect and on every reconnect. Pulls the widget positions down so
// a device that rebooted does not sit in automatic while the app shows "on".
BLYNK_CONNECTED() {
  Serial.println(F("Blynk: connected, syncing widgets"));
  Blynk.syncVirtual(VP_FAN_MODE, VP_LIGHT_MODE, VP_SETPOINT);
}

void pushBlynk() {
  if (!Blynk.connected()) return;

  Blynk.virtualWrite(VP_TEMP, tempC);
  Blynk.virtualWrite(VP_HUMIDITY, humidity);
  Blynk.virtualWrite(VP_LIGHT, lightPct);
  Blynk.virtualWrite(VP_OCCUPIED, (int)occupied);
  Blynk.virtualWrite(VP_LIGHTS, (int)lightsOn);
  Blynk.virtualWrite(VP_FAN_SPEED, fanSpeedPct);
  Blynk.virtualWrite(VP_ALARM, (int)alarmOn);
  Blynk.virtualWrite(VP_CONTROL_SRC, (int)fanOverride * 10 + (int)lightOverride);
}

// ------------------------------------------------------------ remote control

// Pops one command off the TalkBack queue. ThingSpeak answers with the command
// text, or an empty body when the queue is empty.
void pollTalkBack() {
  if (TB_ID == 0UL || WiFi.status() != WL_CONNECTED) return;

  char url[160];
  snprintf(url, sizeof(url),
           "http://api.thingspeak.com/talkbacks/%lu/commands/execute?api_key=%s",
           (unsigned long)TB_ID, TB_API_KEY);

  HTTPClient http;
  if (!http.begin(tbNet, url)) return;

  int code = http.GET();
  if (code == 200) {
    String cmd = http.getString();
    cmd.trim();
    if (cmd.length()) applyCommand(cmd);
  } else if (code > 0) {
    Serial.printf("TalkBack: HTTP %d\r\n", code);
  }
  http.end();
}

void applyCommand(const String &raw) {
  String cmd = raw;
  cmd.toUpperCase();
  Serial.printf("TalkBack: command '%s'\r\n", cmd.c_str());

  if (cmd == "FAN_ON") {
    fanOverride = OV_FORCE_ON;
    fanOverrideMs = millis();
  } else if (cmd == "FAN_OFF") {
    fanOverride = OV_FORCE_OFF;
    fanOverrideMs = millis();
  } else if (cmd == "FAN_AUTO") {
    fanOverride = OV_AUTO;
  } else if (cmd == "LIGHTS_ON") {
    lightOverride = OV_FORCE_ON;
    lightOverrideMs = millis();
  } else if (cmd == "LIGHTS_OFF") {
    lightOverride = OV_FORCE_OFF;
    lightOverrideMs = millis();
  } else if (cmd == "LIGHTS_AUTO") {
    lightOverride = OV_AUTO;
  } else if (cmd == "ALL_AUTO") {
    fanOverride = OV_AUTO;
    lightOverride = OV_AUTO;
  } else {
    Serial.println(F("TalkBack: command not recognised, ignoring"));
  }
}

// Overrides are deliberately temporary.
void expireOverrides(unsigned long now) {
  if (fanOverride != OV_AUTO && now - fanOverrideMs >= OVERRIDE_TIMEOUT_MS) {
    fanOverride = OV_AUTO;
    Serial.println(F("Override: fan back to automatic"));
  }
  if (lightOverride != OV_AUTO && now - lightOverrideMs >= OVERRIDE_TIMEOUT_MS) {
    lightOverride = OV_AUTO;
    Serial.println(F("Override: lights back to automatic"));
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
  ThingSpeak.setField(6, fanSpeedPct);
  ThingSpeak.setField(7, (int)alarmOn);
  // Two digit override code: tens digit is the fan, units digit the lights.
  // 0 auto, 1 forced on, 2 forced off. So 12 means fan on, lights off.
  ThingSpeak.setField(8, (int)fanOverride * 10 + (int)lightOverride);
  ThingSpeak.setStatus(inSession ? (occupied ? "class, occupied" : "class, empty")
                                 : "outside teaching hours");

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

  // The module's analog output falls as the room brightens, so the darker
  // reference is subtracted from the sample rather than the other way round.
  ldrRaw = analogRead(PIN_LDR);
  lightPct = 100.0f * (float)(LDR_RAW_DARK - ldrRaw) /
             (float)(LDR_RAW_DARK - LDR_RAW_BRIGHT);
  lightPct = constrain(lightPct, 0.0f, 100.0f);
}

// -------------------------------------------------------------- decisions

void applyRules(unsigned long now) {
  // Lights: only while the room is occupied, and only when it is dark.
  // Hysteresis keeps the LED from chattering around the threshold.
  if (!inSession) {
    lightsOn = false;
  } else if (!occupied) {
    lightsOn = false;
  } else if (!lightsOn && lightPct < DARK_PCT) {
    lightsOn = true;
  } else if (lightsOn && lightPct > DARK_PCT + LIGHT_HYST_PCT) {
    lightsOn = false;
  }

  // Fan: occupied and warm. Speed rises with how far the room has overshot
  // the setpoint instead of slamming between off and full.
  if (!inSession || !occupied) {
    fanOn = false;
  } else if (!isnan(tempC)) {
    if (!fanOn && tempC > fanOnC) {
      fanOn = true;
    } else if (fanOn && tempC < fanOnC - FAN_HYST_C) {
      fanOn = false;
    }
  }
  fanSpeedPct = fanOn ? demandedSpeed(tempC) : 0;

  // Remote commands override everything decided above, then time out.
  expireOverrides(now);

  if (lightOverride == OV_FORCE_ON)  lightsOn = true;
  if (lightOverride == OV_FORCE_OFF) lightsOn = false;

  if (fanOverride == OV_FORCE_ON) {
    fanOn = true;
    fanSpeedPct = FAN_FORCE_SPEED_PCT;
  } else if (fanOverride == OV_FORCE_OFF) {
    fanOn = false;
    fanSpeedPct = 0;
  }

  // Alarm: temperature emergency, regardless of occupancy.
  alarmOn = !isnan(tempC) && tempC > ALARM_C;

  digitalWrite(PIN_LIGHT, lightsOn ? HIGH : LOW);
  digitalWrite(PIN_RELAY, fanOn ? HIGH : LOW);
  ledcWrite(PIN_FAN_PWM, (fanSpeedPct * 255) / 100);

  if (!alarmOn) {
    digitalWrite(PIN_BUZZER, LOW);
    beepHigh = false;
    lastBeepMs = now;
  }
}

// Linear ramp from FAN_ON_C to FAN_FULL_C, floored at the minimum duty the
// motor can actually start on.
int demandedSpeed(float t) {
  if (isnan(t)) return FAN_MIN_DUTY_PCT;

  float span = fanFullC - fanOnC;
  float pct = 100.0f * (t - fanOnC) / span;
  pct = constrain(pct, 0.0f, 100.0f);

  int duty = (int)(pct + 0.5f);
  return duty < FAN_MIN_DUTY_PCT ? FAN_MIN_DUTY_PCT : duty;
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
  display.print(clockText);
  display.print(inSession ? F(" CLASS") : F(" CLOSED"));
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
  display.print(lightsOn ? F("ON") : F("OFF"));
  if (lightOverride != OV_AUTO) display.print('*');
  display.print(F(" FAN:"));
  if (fanOverride != OV_AUTO) display.print('*');
  if (fanSpeedPct > 0) {
    display.print(fanSpeedPct);
    display.print('%');
  } else {
    display.print(F("OFF"));
  }
  if (alarmOn) display.print(F(" !"));

  display.display();
}

void logSerial() {
  // \r\n keeps the line aligned in serial views that do not translate \n
  Serial.printf("%s %s T=%.1fC H=%.0f%% L=%.0f%% (adc %4d) motion=%d "
                "occupied=%d lights=%d fan=%d%% alarm=%d\r\n",
                clockText, inSession ? "class " : "closed",
                tempC, humidity, lightPct, ldrRaw, motion, occupied,
                lightsOn, fanSpeedPct, alarmOn);
}
