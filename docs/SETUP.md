# Setup

Two ways to run this: the Wokwi website (no install) or the Wokwi VS Code extension (local build, faster iteration).

## Option A: Wokwi website

1. Open https://wokwi.com/projects/new/esp32.
2. Replace the editor contents of `sketch.ino` with this repo's `sketch/sketch.ino`.
3. Add three more files with the `+` tab button and paste in `sketch/config.h`, `sketch/secrets.h` and `diagram.json`.
4. Open the Library Manager tab and add each entry from `libraries.txt`:
   - DHT sensor library
   - Adafruit Unified Sensor
   - Adafruit GFX Library
   - Adafruit SSD1306
   - ThingSpeak
5. Press the green play button.

`Wokwi-GUEST` is Wokwi's built in access point, so the ESP32 reaches the real internet from the simulation and ThingSpeak updates land in your channel.

## Option B: Wokwi VS Code extension

1. Install the extensions:
   - `wokwi.wokwi-vscode`
   - `vsciot-vscode.vscode-arduino` (or use arduino-cli directly)
2. Install arduino-cli and the ESP32 core:
   ```powershell
   winget install ArduinoSA.CLI
   arduino-cli config init
   arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
   arduino-cli core update-index
   arduino-cli core install esp32:esp32
   arduino-cli lib install "DHT sensor library" "Adafruit Unified Sensor" "Adafruit GFX Library" "Adafruit SSD1306" "ThingSpeak"
   ```
3. Build into the path `wokwi.toml` expects:
   ```powershell
   arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir build sketch
   ```
   The firmware lives in `sketch/` because arduino-cli requires the sketch folder
   and the main `.ino` file to share a name. Wokwi reads `diagram.json` and
   `wokwi.toml` from the repo root, so the nesting does not affect it.

   Verified output on ESP32 core 3.x:
   ```
   Sketch uses 940864 bytes (71%) of program storage space.
   Global variables use 48760 bytes (14%) of dynamic memory.
   ```
4. Run the Wokwi extension command `Wokwi: Start Simulator`. It gets a free licence key on first use.

## ThingSpeak channel

1. Sign in at https://thingspeak.com and create a new channel.
2. Enable seven fields and name them:

   | Field | Name |
   |---|---|
   | 1 | Temperature C |
   | 2 | Humidity % |
   | 3 | Light % |
   | 4 | Occupied |
   | 5 | Lights |
   | 6 | Fan speed % |
   | 7 | Alarm |
   | 8 | Override code |

3. Save, then open the API Keys tab and copy the Channel ID and the Write API Key.
4. Put both into `secrets.h`:
   ```c
   #define TS_CHANNEL_ID    1234567UL
   #define TS_WRITE_API_KEY "XXXXXXXXXXXXXXXX"
   ```
5. Restart the simulation. The serial monitor prints `ThingSpeak: update accepted` every 15 seconds and the OLED header shows `TS`.

The free tier rejects updates faster than one per 15 seconds, which is why `TS_PERIOD_MS` is 15000. Lower it only on a paid plan.

## TalkBack remote control

TalkBack is a command queue hosted by ThingSpeak. The device pops one command from it every 20 seconds, which is how the fan and lights can be driven from a browser.

1. In ThingSpeak open **Apps**, then **TalkBack**, then **New TalkBack**.
2. Name it `Classroom control` and save. Leave the command list empty for now.
3. Copy the **TalkBack ID** and the **API Key** shown at the top.
4. Put them into `sketch/secrets.h`:
   ```c
   #define TB_ID          54321UL
   #define TB_API_KEY     "XXXXXXXXXXXXXXXX"
   ```
5. Rebuild and restart the simulation.

To send a command, either use **Add TalkBack Command** on the TalkBack page, or open this URL in any browser:

```
https://api.thingspeak.com/talkbacks/<TB_ID>/commands.json?api_key=<TB_API_KEY>&command_string=FAN_ON
```

Accepted commands:

| Command | Effect |
|---|---|
| `FAN_ON` | fan forced to full speed |
| `FAN_OFF` | fan forced off |
| `FAN_AUTO` | fan returns to temperature control |
| `LIGHTS_ON` | lights forced on |
| `LIGHTS_OFF` | lights forced off |
| `LIGHTS_AUTO` | lights return to sensor control |
| `ALL_AUTO` | clears both overrides |

Overrides beat the sensors and the timetable, and revert on their own after `OVERRIDE_TIMEOUT_MS`, five minutes by default. Leaving `TB_ID` at 0 disables the whole feature and the device never calls out.

## Timetable

`config.h` holds the schedule. Defaults are 08:30 to 17:00, Monday to Friday, on IST (`TZ_OFFSET_SEC` 19800).

- `TZ_OFFSET_SEC` seconds east of UTC, so 0 for UTC, 3600 for CET
- `SCHOOL_START_MIN` and `SCHOOL_END_MIN` as minutes past midnight
- `SCHOOL_DAY_MASK` one bit per weekday, bit 0 is Sunday, `0x3E` is Monday to Friday, `0x7F` is every day

If NTP never answers, the schedule fails open and automation runs as if class were in session. An unsynced clock should not leave a room dark.

## Tuning

All thresholds live in `config.h`. The ones you are most likely to touch:

- `DARK_PCT` - light level below which the lights switch on
- `FAN_ON_C` - temperature that starts the fan relay
- `ALARM_C` - temperature that starts the buzzer
- `VACANCY_MS` - how long after the last motion the room counts as empty
