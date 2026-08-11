#pragma once

// ---------------------------------------------------------------- pin map
#define PIN_DHT      15   // DHT22 data
#define PIN_LDR      34   // photoresistor analog out (ADC1_CH6, input only)
#define PIN_PIR      13   // HC-SR501 motion out
#define PIN_RELAY    26   // relay module IN, isolation stage for the fan circuit
#define PIN_FAN_PWM  32   // LEDC output, fan speed, routed through the relay contacts
#define PIN_LIGHT    25   // classroom lights LED
#define PIN_BUZZER   27   // alarm buzzer
#define PIN_SDA      21   // OLED I2C
#define PIN_SCL      22

// ---------------------------------------------------------- display setup
#define OLED_WIDTH   128
#define OLED_HEIGHT  64
#define OLED_ADDR    0x3C

// ------------------------------------------------------ automation limits
// Ambient light is reported as 0..100 %. Below DARK_PCT the room is dark.
#define DARK_PCT           35.0f
#define LIGHT_HYST_PCT      8.0f   // must climb this far past DARK_PCT to turn lights off

// Fan control is proportional, not on/off. The fan starts at FAN_ON_C and
// reaches full speed at FAN_FULL_C, with the duty cycle interpolated between.
// It stops only after falling below FAN_ON_C - FAN_HYST_C.
#define FAN_ON_C           28.0f
#define FAN_FULL_C         34.0f
#define FAN_HYST_C          1.5f

// A real fan will not start from rest at a low duty cycle, so any non zero
// demand is lifted to at least this much.
#define FAN_MIN_DUTY_PCT     30

// LEDC settings for the speed output.
#define FAN_PWM_FREQ      20000   // above hearing range, no audible whine
#define FAN_PWM_BITS          8   // 0..255 duty

// Buzzer sounds while temperature stays above this.
#define ALARM_C            35.0f

// Room counts as empty this long after the last motion pulse.
#define VACANCY_MS         10000UL

// ---------------------------------------------------------- class timetable
// Wall clock time comes from NTP. Offset is seconds east of UTC:
// 19800 is IST (UTC+5:30). Use 0 for UTC, 3600 for CET, and so on.
#define TZ_OFFSET_SEC      19800
#define TZ_DST_SEC             0
#define NTP_SERVER_A       "pool.ntp.org"
#define NTP_SERVER_B       "time.nist.gov"

// Teaching hours as minutes past midnight.
#define SCHOOL_START_MIN   (8 * 60 + 30)    // 08:30
#define SCHOOL_END_MIN     (17 * 60)        // 17:00

// One bit per weekday, bit 0 is Sunday. 0x3E is Monday through Friday.
// Add Saturday with 0x7E, make it every day with 0x7F.
#define SCHOOL_DAY_MASK    0x3E

// Outside teaching hours lights and fan are held off no matter what the
// sensors say. The over temperature alarm stays armed around the clock.

// ------------------------------------------------------------- timing
#define SENSOR_PERIOD_MS    2000UL   // DHT22 needs >= 2 s between reads
#define DISPLAY_PERIOD_MS    500UL
#define BEEP_PERIOD_MS       600UL   // alarm on/off half cycle
#define TALKBACK_PERIOD_MS 20000UL   // how often the command queue is polled

// A remote override reverts to automatic control after this long, so a
// forgotten "fan on" command cannot run the fan all night.
#define OVERRIDE_TIMEOUT_MS 300000UL  // 5 minutes

// Speed used when the fan is forced on from the cloud.
#define FAN_FORCE_SPEED_PCT   100

// ------------------------------------------------------- ADC calibration
// Raw ADC counts measured at each end of the Wokwi illumination slider.
// The module divides the LDR against a fixed resistor such that the analog
// output RISES as the room gets darker, so the dark value is the larger one.
// Measured on the Wokwi photoresistor module: 0 lux -> 4063, max lux -> 32.
#define LDR_RAW_DARK       4063
#define LDR_RAW_BRIGHT       32
