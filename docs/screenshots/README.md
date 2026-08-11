# Captured screenshots

Numbering follows [../SCREENSHOTS.md](../SCREENSHOTS.md). All captured from the Wokwi VS Code simulator on 2026-08-11 between 20:22 and 20:49.

| File | Shows |
|---|---|
| `01-circuit-diagram.png` | Full canvas, all 11 parts wired: ESP32, OLED, DHT22, photoresistor, PIR, relay, two LEDs, two resistors, buzzer. |
| `02-boot-wifi-ntp.png` | Boot sequence. `WiFi: connected, IP 10.13.37.2` then `Clock: 2026-08-11 20:32:53 local`, proving both network and NTP work from inside the simulation. |
| `03-baseline-oled.png` | OLED at rest: 24.0 C, 55 %, `Light 76 %`, `Room EMPTY`, `LGT:OFF FAN:OFF`. Header reads `CLOSED` because this was taken before teaching hours were widened. |
| `04-motion-detected.png` | `motion=1 occupied=1` after pressing Simulate motion, with the room still lit at `L=76%`. |
| `05-lights-on-dark-occupied.png` | The lighting rule firing: `class`, `L=0%`, `occupied=1`, `lights=1`, yellow LIGHTS LED visibly lit. |
| `06-fan-on-50pct.png` | Proportional fan at 31.0 C: `fan=50%`, blue FAN LED lit at partial brightness, relay closed. |
| `08-fan-ramp-100pct-alarm.png` | The ramp and the alarm together. `fan=50%` at 31.0 C, then `fan=100% alarm=1` at 47.9 C, with the buzzer sounding. |
| `09-timetable-lockout-occupied.png` | Outside teaching hours, room dark, hot and **occupied**: `occupied=1 lights=0 fan=0%`. Proves the timetable suppressed the loads, not the occupancy rule. |
| `09b-timetable-lockout-alarm.png` | Same lockout at 39.4 C, `alarm=1`. Safety ignores the timetable while comfort loads stay off. |
| `09c-timetable-lockout-sweep.png` | Temperature swept 29.9 to 39.4 C with `fan=0%` at every step, because the room is closed. |
| `09d-timetable-lockout-occupied.png` | Lockout at 24 C, dark and occupied, everything off. |
| `10-remote-override-fan-on.png` | `TalkBack: command 'FAN_ON'` followed by `closed ... occupied=0 ... fan=100%`. The remote override outranking both the sensors and the timetable. |
| `11a-thingspeak-fields-1-4.png` | Cloud dashboard, temperature, humidity, light and occupancy. The temperature ramp 25 to 31 to 47 C, the light sweep, and the occupancy pulses all line up in time. |
| `11b-thingspeak-fields-5-6.png` | Lights and fan speed. Field 6 is the important one: a **staircase** at 100, then 50, then 100. A bang-bang controller would draw a square wave, the intermediate step is the proportional control. |
| `11c-thingspeak-fields-7-8.png` | Alarm and override code. Field 8 shows code 10, fan forced on, dropping back to 0 when the override expired. |

## Reading the charts together

The 20:15 block in field 6 at 100 % with field 8 at 10 is the remote override. The 20:47 activity at 50 then 100 with field 8 at 0 is the temperature rule driving the fan on its own. Same actuator, two different sources of authority, distinguishable from the cloud data alone.

## Worth retaking

- `03-baseline-oled.png` shows `CLOSED` in the header, since it predates the widened schedule. A `CLASS` version would match the other in-session shots, though the current one doubles as evidence that the schedule was active.
