# Runbook

End to end walkthrough: create the ThingSpeak channel, load the project into Wokwi, run the demo.

---

## Part 1: ThingSpeak channel

1. Go to https://thingspeak.com and click **Sign In**. ThingSpeak uses a MathWorks account. If you do not have one, click **Create one!** on the sign in page and register with your email. The free tier allows 4 channels and 3 million messages per year, which is far beyond what this project needs.

2. After signing in you land on **My Channels**. Click the **New Channel** button.

3. Fill in the channel form:
   - **Name**: `Smart Classroom`
   - **Description**: `ESP32 classroom telemetry from Wokwi` (optional)

4. Tick the checkbox next to **Field 1** through **Field 7** and type these names. The checkbox must be ticked or the field is not created, and the firmware writes all seven at once.

   | Field | Name |
   |---|---|
   | 1 | Temperature C |
   | 2 | Humidity % |
   | 3 | Light % |
   | 4 | Occupied |
   | 5 | Lights |
   | 6 | Fan |
   | 7 | Alarm |

5. Leave everything else at its default and click **Save Channel** at the bottom.

6. You are now on the channel page with tabs: Private View, Public View, Channel Settings, Sharing, API Keys, Data Import / Export. Two values are needed:
   - **Channel ID**: shown directly under the channel name at the top of the page, a 7 digit number.
   - **Write API Key**: click the **API Keys** tab. The first box is **Write API Key**, a 16 character string. Copy it. Ignore the Read API Key, the firmware does not need it.

7. Open `sketch/secrets.h` and replace the two placeholders. Keep the `UL` suffix on the channel ID and keep the API key in quotes:

   ```c
   #define TS_CHANNEL_ID    2891234UL
   #define TS_WRITE_API_KEY "A1B2C3D4E5F6G7H8"
   ```

   Leave `WIFI_SSID`, `WIFI_PASSWORD` and `WIFI_CHANNEL` alone. `Wokwi-GUEST` is the simulator's built in access point and it reaches the real internet, so ThingSpeak receives real data from the simulation.

If you skip this part the project still runs. The serial monitor prints `ThingSpeak: channel not configured, skipping upload` every 15 seconds and everything else works.

---

## Part 2: load the project into Wokwi

1. Go to https://wokwi.com and click **Sign in** at the top right. Signing in with GitHub is enough. Anonymous sessions get cut short, a signed in session runs long enough to walk the whole demo.

2. Open https://wokwi.com/projects/new/esp32. You get a blank ESP32 project with two tabs above the editor: `sketch.ino` and `diagram.json`, plus a **Library Manager** tab.

3. Click the **sketch.ino** tab. Select all the existing code and delete it. Paste the full contents of this repo's `sketch/sketch.ino`.

4. Click the **diagram.json** tab. Select all and delete. Paste the full contents of this repo's `diagram.json`. The canvas on the right redraws with the ESP32, DHT22, photoresistor, PIR, OLED, relay, two LEDs, two resistors and the buzzer already wired.

5. Add the two headers. Click the **+** button on the tab bar, choose **New File**, name it exactly `config.h`, and paste this repo's `sketch/config.h`. Repeat for `secrets.h` with `sketch/secrets.h`, including the ThingSpeak values from Part 1.

   The file names must match exactly. `sketch.ino` does `#include "config.h"` and `#include "secrets.h"`.

6. Click the **Library Manager** tab, then the **+** button, and add these five one at a time by searching the name and clicking it:

   - `DHT sensor library`
   - `Adafruit Unified Sensor`
   - `Adafruit GFX Library`
   - `Adafruit SSD1306`
   - `ThingSpeak`

   Wokwi writes them into a `libraries.txt` tab as you go. It should end up matching this repo's `libraries.txt`.

7. Press the green **play** button. The first build takes 20 to 60 seconds. When it starts you should see:

   - the OLED showing `Smart Classroom / starting up...`, then the live status screen
   - the serial monitor at the bottom printing the WiFi join, then one telemetry line every 2 seconds

8. Expected first serial output:

   ```
   Smart Classroom Automation booting
   WiFi: joining Wokwi-GUEST
   ....
   WiFi: connected, IP 10.10.0.2
   T=24.0C H=55% L=100% motion=0 occupied=1 lights=0 fan=0 alarm=0
   ```

   `occupied=1` at the very start is expected. The vacancy timer starts at boot, so the room reads occupied for the first 10 seconds and then falls to `occupied=0`.

9. If the OLED stays blank, check that the serial monitor does not say `SSD1306 not found`. If it does, the `oled` wiring in `diagram.json` did not paste cleanly. Re-paste `diagram.json`.

---

## Part 3: run the demo

Each step is a visible change on the canvas, the OLED and the ThingSpeak charts. Interactive parts only respond while the simulation is running.

### Step 0: baseline

Let it settle for about 20 seconds without touching anything.

Expected: OLED reads `Temp 24.0 C`, `Hum 55 %`, `Light 100 %`, `Room EMPTY`, `LGT:OFF FAN:OFF`. Both LEDs dark, buzzer silent. Serial shows `occupied=0`. The header corner shows `TS` after the first accepted upload.

Open your ThingSpeak channel's **Private View** in a second browser tab. The first data point appears within 15 seconds. Charts refresh on page reload, or use the refresh icon on each chart.

### Step 1: detect motion

Click the **PIR sensor** body on the canvas. Wokwi fires a motion pulse for a couple of seconds.

Expected:
- OLED `Room` flips to `OCCUPIED`
- serial shows `motion=1 occupied=1`
- 10 seconds after the last click it falls back to `EMPTY`
- ThingSpeak field 4 steps to 1

Keep clicking the PIR every few seconds during the next steps to hold the room occupied. This is deliberate: the automation only acts on an occupied room.

### Step 2: change light level

Click the **photoresistor** part. A light slider appears next to it. Drag it toward the dark end while keeping motion alive with PIR clicks.

Expected:
- the `Light %` line on the OLED falls
- once it drops below 35 the yellow **LIGHTS** LED turns on and `LGT:ON` appears
- drag the slider back up: the LED does not turn off at 35, it waits until light climbs past 43. That 8 point gap is the hysteresis band in `LIGHT_HYST_PCT`, it stops the LED flickering at the threshold
- ThingSpeak field 3 tracks the light level, field 5 steps to 1

Now stop clicking the PIR and wait 10 seconds with the room still dark. The lights turn off because the room is empty. That pairing is the point of using the LDR and the PIR together.

If the `Light %` reading does not span roughly 0 to 100 as you drag, adjust `LDR_RAW_DARK` and `LDR_RAW_BRIGHT` in `config.h` to the raw ADC values you actually see, then restart the simulation.

### Step 3: increase temperature

Click the **DHT22** part. Temperature and humidity sliders appear. Drag temperature above 28 C while keeping motion alive.

Expected:
- the relay module clicks and its onboard LED lights
- the blue **FAN** LED turns on, OLED shows `FAN:ON`
- drop the temperature back down: the fan stays on until 26.5 C, which is `FAN_ON_C` minus `FAN_HYST_C`
- ThingSpeak fields 1 and 6 follow

Readings only refresh every 2 seconds because the DHT22 cannot be polled faster, so allow a moment after each slider move.

### Step 4: trigger the alarm

Keep dragging the DHT22 temperature past 35 C.

Expected:
- the buzzer beeps on and off at roughly one cycle per 1.2 seconds
- a `!` appears at the end of the OLED status row
- serial shows `alarm=1`
- ThingSpeak field 7 steps to 1

The alarm ignores occupancy on purpose, an overheating empty room still needs to raise a flag. Cool the room below 35 C and the buzzer stops.

---

## What to capture for a report

- Serial monitor log covering all four transitions
- OLED screenshots at baseline, lights on, fan on, alarm active
- ThingSpeak Private View with all seven charts populated
- The Wokwi project URL, use **Save** in Wokwi to get a shareable link

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| `ThingSpeak: write failed, HTTP -301` | not connected | check the serial log for a successful WiFi join, restart the simulation |
| `ThingSpeak: write failed, HTTP 400` | wrong channel ID or key | recopy both from the API Keys tab |
| Uploads accepted but charts empty | looking at Public View | switch to Private View, or make fields public under Sharing |
| Fan LED on when it should be off | relay module treated as active low | invert the two `digitalWrite(PIN_RELAY, ...)` calls in `sketch.ino` |
| Lights never switch on | light never reads below 35 | recalibrate `LDR_RAW_DARK` and `LDR_RAW_BRIGHT` |
| Nothing reacts to clicks | simulation not running | press play, interactive parts are inert while stopped |
