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
| Relay module | fan control | GPIO 26 |
| LED + 220R | classroom lights | GPIO 25 |
| Buzzer | over temperature alarm | GPIO 27 |

## Automation rules

1. Lights turn ON when motion is detected and ambient light is below the dark threshold.
2. Lights turn OFF after the room stays empty for the vacancy timeout.
3. Fan relay turns ON when the room is occupied and temperature rises above the fan threshold, OFF below the threshold minus hysteresis.
4. Buzzer beeps while temperature exceeds the alarm threshold.
5. All five readings plus actuator states are published to ThingSpeak every 15 seconds.

## Repository layout

```
sketch.ino       firmware
config.h         thresholds, pins, WiFi and ThingSpeak settings
diagram.json     Wokwi wiring
libraries.txt    Wokwi library dependencies
wokwi.toml       Wokwi VS Code extension config
docs/            setup and demo notes
```

## Quick start

See [docs/SETUP.md](docs/SETUP.md).
