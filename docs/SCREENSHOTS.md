# Screenshot checklist

Eight images cover the whole project: circuit, each automation rule firing, and the cloud dashboard. Capture them in this order and the simulation state flows from one to the next without restarts.

On Windows use `Win + Shift + S` for a region capture, then paste into your report. `Win + PrtScn` grabs the whole screen and auto-saves to `Pictures\Screenshots`.

Before starting: restart the simulator so the log begins at boot, and leave the illumination slider at its default 500 lux.

---

## 1. Circuit diagram

**State**: simulation stopped or running, whole canvas visible.

1. Zoom out until all 11 parts fit: ESP32, OLED, DHT22, photoresistor, PIR, relay, two LEDs, two resistors, buzzer.
2. Capture the canvas only, no terminal.

Shows the full wiring. Caption it with the pin table from the README.

## 2. Boot and WiFi join

**State**: first seconds after pressing play.

1. Restart the simulator.
2. Wait until the first telemetry lines appear.
3. Capture the terminal, including these lines:

```
Smart Classroom Automation booting
WiFi: joining Wokwi-GUEST
WiFi: connected, IP 10.13.37.2
T=24.0C H=55% L=76% (adc 1001) motion=0 occupied=1 lights=0 fan=0 alarm=0
```

Proves the ESP32 reaches the network from inside the simulation.

## 3. Baseline OLED

**State**: room empty, default light, default temperature. Wait 15 seconds after boot so occupancy clears.

1. Zoom the canvas in on the OLED until the text is readable.
2. Capture just the display.

Expected: `Temp 24.0 C`, `Hum 55 %`, `Light 76 %`, `Room EMPTY`, `LGT:OFF FAN:OFF`, and `TS` in the top right once the first upload is accepted.

## 4. Motion detected

**State**: occupancy triggered, nothing else changed.

1. Click the PIR sensor, press **Simulate motion**.
2. Within 2 seconds capture the terminal and the OLED together, or take two shots.

Expected: serial `motion=1 occupied=1`, OLED `Room OCCUPIED`.

## 5. Lights ON, dark and occupied

**State**: occupied plus dark.

1. Keep pressing **Simulate motion** every few seconds.
2. Click the photoresistor and drag `ILLUMINATION (LUX)` to the far left.
3. Wait for the yellow LIGHTS LED to light.
4. Capture the canvas region containing the LED, the LDR slider and the OLED.

Expected: serial `L=0% (adc 4063) ... lights=1`, OLED `LGT:ON`.

Worth a second shot: drag the slider back up and capture the moment lights are still on above 35 %, which demonstrates the hysteresis band.

## 6. Fan ON, hot and occupied

**State**: occupied plus warm. Light can stay dark or go back to normal.

1. Keep pressing **Simulate motion**.
2. Click the DHT22 and drag **Temperature** to about 30 C.
3. Wait 2 seconds for the next sensor read.
4. Capture the relay module, the blue FAN LED and the terminal.

Expected: serial `T=30.0C ... fan=1`, relay module LED lit, blue FAN LED lit, OLED `FAN:ON`.

## 7. Alarm

**State**: over temperature.

1. Drag the DHT22 temperature above 35 C, for example 39 C.
2. Capture the buzzer (a sound icon appears next to it), the OLED showing `!` on the status row, and the terminal.

Expected: serial `alarm=1`, buzzer beeping about once a second.

Point worth making in the report: the alarm fires even with `occupied=0`, because an overheating empty room still needs attention, while the fan and lights stay off.

## 8. Fan speed ramp

**State**: occupied, temperature swept upward.

1. Hold motion and step the DHT22 temperature through 29 C, 31 C, 34 C, pausing 2 seconds at each.
2. Capture the terminal showing three or four lines with rising `fan=` percentages, alongside the blue LED.

Expected: `fan=30%`, then roughly `fan=50%`, then `fan=100%`, with the LED visibly brighter at each step. This is the shot that proves proportional control rather than on/off.

## 9. Timetable lockout

**State**: outside teaching hours.

1. Either run after 17:00, or set `SCHOOL_END_MIN` in `config.h` a minute or two ahead of the current clock and rebuild.
2. Wait for the header to flip to `CLOSED`.
3. Hold motion, make the room dark and hot.
4. Capture the OLED and the terminal together.

Expected: `closed` in every serial line, `lights=0 fan=0%` despite dark, hot and occupied, and the alarm still firing if you push past 35 C.

## 10. Remote override

**State**: TalkBack configured.

1. Queue `FAN_ON` from the browser.
2. Capture the browser tab or TalkBack page showing the queued command.
3. Within 20 seconds capture the terminal line `TalkBack: command 'FAN_ON'` plus the OLED showing `FAN:*100%`.

Expected: fan at full speed in an empty room, asterisk on the OLED, field 8 reading 10 on ThingSpeak. Two images, the command going out and the device obeying.

## 11. ThingSpeak dashboard

**State**: after running steps 2 through 7, so every field has movement.

1. Open your channel and select **Private View**.
2. For a clean timeline, use **Channel Settings** then **Clear Channel** before the demo run, otherwise older data is mixed in.
3. Capture fields 1 to 4 in one shot and fields 5 to 7 in a second, or zoom the browser out to fit all seven.

Expected shapes: field 1 steps up at the temperature test, field 3 dips to 0 during the dark test, fields 4 to 7 are square pulses.

---

## Optional extras

- **Local build proof**: terminal showing `arduino-cli compile` output with the flash and RAM figures.
- **Wokwi VS Code**: the editor with `diagram.json`, `wokwi.toml` and the simulator panel side by side.
- **Repository**: the GitHub commit history.
