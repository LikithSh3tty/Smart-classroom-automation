# Demo script

Run the simulation, then walk these four steps. Each one is a visible state change on the OLED, the LEDs and the ThingSpeak charts.

## 0. Baseline

Start state: 24 C, 55 % humidity, bright room, no motion. OLED reads `EMPTY`, both LEDs off, no buzzer.

Wait about 15 seconds so the first ThingSpeak point lands.

## 1. Detect motion

Click the PIR sensor in the diagram, then press **Simulate motion** in the panel that appears.

Expected: OLED flips to `OCCUPIED` and stays there for 10 seconds after the last pulse. Field 4 on ThingSpeak goes to 1.

## 2. Change light level

Drag the slider on the photoresistor down to a dark value while motion is still active.

Expected: `Light %` falls below 35 and the yellow LIGHTS LED turns on. Drag back up past 43 % and it turns off again, the 8 % gap is the hysteresis band that stops flicker.

If the room is empty the lights stay off no matter how dark it gets, which is the point of pairing the LDR with the PIR.

## 3. Increase temperature

Drag the DHT22 temperature slider up past 28 C, keeping motion alive.

Expected: the relay clicks, the blue FAN LED lights dimly and the OLED shows `FAN:30%`. Keep raising the temperature and both the brightness and the percentage climb, hitting 100 % at 34 C. Drop below 26.5 C and it releases.

## 4. Trigger the alarm

Push the temperature past 35 C.

Expected: the buzzer beeps at roughly 1 Hz and a `!` appears on the OLED status row. Field 7 goes to 1. Cool the room back down and it stops.

## 5. Timetable and remote control

Two rules that do not come from the sensors at all:

- The OLED header shows the clock and `CLASS` or `CLOSED`, from NTP. Outside teaching hours the lights and fan are held off however dark, hot or occupied the room is. The alarm still fires.
- A `FAN_ON` command queued on ThingSpeak TalkBack overrides both the sensors and the timetable within 20 seconds, marks itself with an asterisk on the OLED, and expires back to automatic after five minutes.

Full walkthrough for both in [RUNBOOK.md](RUNBOOK.md).

## What to capture for a report

- Serial monitor lines, one per 2 seconds, showing the state transitions
- OLED screenshot in each of the four states
- ThingSpeak channel view with all seven field charts populated
