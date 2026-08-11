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
