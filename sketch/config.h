#pragma once

// ---------------------------------------------------------------- pin map
#define PIN_DHT      15   // DHT22 data
#define PIN_LDR      34   // photoresistor analog out (ADC1_CH6, input only)
#define PIN_PIR      13   // HC-SR501 motion out
#define PIN_RELAY    26   // relay module IN, drives the fan
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

// Fan relay switches on above FAN_ON_C and off below FAN_ON_C - FAN_HYST_C.
#define FAN_ON_C           28.0f
#define FAN_HYST_C          1.5f

// Buzzer sounds while temperature stays above this.
#define ALARM_C            35.0f

// Room counts as empty this long after the last motion pulse.
#define VACANCY_MS         10000UL

// ------------------------------------------------------------- timing
#define SENSOR_PERIOD_MS    2000UL   // DHT22 needs >= 2 s between reads
#define DISPLAY_PERIOD_MS    500UL
#define BEEP_PERIOD_MS       600UL   // alarm on/off half cycle

// ------------------------------------------------------- ADC calibration
// Raw ADC counts measured at each end of the Wokwi illumination slider.
// The module divides the LDR against a fixed resistor such that the analog
// output RISES as the room gets darker, so the dark value is the larger one.
// Measured on the Wokwi photoresistor module: 0 lux -> 4063, max lux -> 32.
#define LDR_RAW_DARK       4063
#define LDR_RAW_BRIGHT       32
