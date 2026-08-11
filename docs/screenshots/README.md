# Captured screenshots

Numbering follows [../SCREENSHOTS.md](../SCREENSHOTS.md).

| File | Shows |
|---|---|
| `09-timetable-lockout-occupied.png` | Outside teaching hours with the room dark, hot and **occupied**. `occupied=1 lights=0 fan=0%` proves the timetable suppressed the loads, not the occupancy rule. The strongest single frame for this feature. |
| `09b-timetable-lockout-alarm.png` | Same lockout at 39.4 C, `alarm=1`. Safety ignores the timetable while comfort loads stay off. |
| `09c-timetable-lockout-sweep.png` | Temperature swept 29.9 to 39.4 C, `fan=0%` at every step because the room is closed. |
| `10-remote-override-fan-on.png` | `TalkBack: command 'FAN_ON'` followed by `closed ... occupied=0 ... fan=100%`. The remote override outranking both the sensors and the timetable. |

Still to capture: see the pending list in [../SCREENSHOTS.md](../SCREENSHOTS.md). Items 3 through 8 need the simulation running inside teaching hours.
