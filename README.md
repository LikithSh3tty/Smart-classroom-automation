# Smart Classroom Automation

ESP32 based classroom controller. Reads temperature, humidity, ambient light and occupancy, then drives lights, a fan and an alarm automatically. Live readings go to an OLED display and to a ThingSpeak channel over WiFi.

Runs fully in simulation on [Wokwi](https://wokwi.com), no physical hardware required.

## Hardware

| Component | Role | ESP32 pin |
|---|---|---|
| DHT22 | temperature + humidity | GPIO 15 |
| LDR (photoresistor) | ambient light level | GPIO 34 (ADC1_CH6) |
| PIR (HC-SR501) | occupancy / motion | GPIO 13 |
| SSD1306 OLED 128x64 | status display | GPIO 21 SDA, GPIO 22 SCL |
| Relay module | fan isolation stage | GPIO 26 |
| Fan speed PWM | LEDC output through the relay contacts | GPIO 32 |
| LED + 220R | classroom lights | GPIO 25 |
| Buzzer | over temperature alarm | GPIO 27 |

## Automation rules

1. Lights turn ON when motion is detected and ambient light is below the dark threshold.
2. Lights turn OFF after the room stays empty for the vacancy timeout.
3. Fan speed is proportional to temperature, ramping from the start threshold to full speed, driven by LEDC PWM through the relay contacts. The relay is the isolation stage, the PWM is the speed controller.
4. Buzzer beeps while temperature exceeds the alarm threshold, occupied or not.
5. Outside teaching hours the lights and fan are held off regardless of the sensors. Wall clock time comes from NTP and the timetable lives in `config.h`.
6. Commands pushed to a ThingSpeak TalkBack queue override the fan and the lights from anywhere, and expire back to automatic after five minutes.
7. Readings, actuator states and the override code are published to ThingSpeak every 15 seconds.

## Control hierarchy

Each layer beats the one above it:

```
sensors and thresholds      lights when dark, fan speed from temperature
        v
timetable                   outside teaching hours, comfort loads are off
        v
remote override             TalkBack command wins over both, then times out
```

The alarm sits outside this hierarchy. It follows temperature alone, because an overheating empty room out of hours still needs to raise a flag.

## Repository layout

```
sketch/sketch.ino  firmware
sketch/config.h    pin map, thresholds and timing
sketch/secrets.h   WiFi credentials and ThingSpeak channel
diagram.json     Wokwi wiring
libraries.txt    Wokwi library dependencies
wokwi.toml       Wokwi VS Code extension config
docs/RUNBOOK.md  full walkthrough, ThingSpeak to demo, with troubleshooting
docs/SETUP.md    how to run it and how to wire up ThingSpeak
docs/DEMO.md     four step demo script
```

## ThingSpeak fields

| Field | Value |
|---|---|
| 1 | temperature C |
| 2 | humidity % |
| 3 | ambient light % |
| 4 | occupied (0/1) |
| 5 | lights (0/1) |
| 6 | fan speed (0 to 100 %) |
| 7 | alarm (0/1) |
| 8 | override code, tens digit fan, units digit lights, 0 auto 1 on 2 off |

## Quick start

Follow [docs/RUNBOOK.md](docs/RUNBOOK.md) start to finish. [docs/SETUP.md](docs/SETUP.md) covers the local arduino-cli build, [docs/DEMO.md](docs/DEMO.md) is the short demo checklist.
