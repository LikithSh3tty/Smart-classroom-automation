#pragma once

// Wokwi's simulated access point. Channel 6 makes the join instant.
// Swap these for a real network when flashing physical hardware.
#define WIFI_SSID      "Wokwi-GUEST"
#define WIFI_PASSWORD  ""
#define WIFI_CHANNEL   6

// ThingSpeak channel that receives the telemetry.
#define TS_CHANNEL_ID    3451260UL
#define TS_WRITE_API_KEY "9ZPIO143AL8TEDKO"

// ThingSpeak free tier accepts one update every 15 seconds.
#define TS_PERIOD_MS   15000UL

// Blynk template and device. Leave BLYNK_AUTH_TOKEN empty to disable Blynk
// entirely, the rest of the firmware runs unchanged without it.
#define BLYNK_TEMPLATE_ID   "TMPL330hrqg9Z"
#define BLYNK_TEMPLATE_NAME "Classroom automation"
#define BLYNK_AUTH_TOKEN    "9_JBMi-MzGUCgR_VxhCu5h1i0pBmdw4S"

// ThingSpeak TalkBack queue, used to push commands down to the device.
// Create one under Apps, TalkBack, New TalkBack. Leave the ID at 0 to
// disable remote control entirely.
// Accepted commands: FAN_ON, FAN_OFF, FAN_AUTO,
//                    LIGHTS_ON, LIGHTS_OFF, LIGHTS_AUTO, ALL_AUTO
#define TB_ID          57533UL
#define TB_API_KEY     "2C9KOORGJRR6AYSQ"
