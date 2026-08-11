#pragma once

// Wokwi's simulated access point. Channel 6 makes the join instant.
// Swap these for a real network when flashing physical hardware.
#define WIFI_SSID      "Wokwi-GUEST"
#define WIFI_PASSWORD  ""
#define WIFI_CHANNEL   6

// ThingSpeak channel that receives the telemetry.
// Replace both values with your own channel, then re-run the simulation.
#define TS_CHANNEL_ID    0UL
#define TS_WRITE_API_KEY "YOUR_THINGSPEAK_WRITE_API_KEY"

// ThingSpeak free tier accepts one update every 15 seconds.
#define TS_PERIOD_MS   15000UL
