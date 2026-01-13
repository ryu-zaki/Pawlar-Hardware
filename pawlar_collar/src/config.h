#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// --- BLYNK ---
#define BLYNK_TEMPLATE_ID   "TMPL6QodO-XJ1"
#define BLYNK_TEMPLATE_NAME "Pawlar Collar"
#define BLYNK_AUTH_TOKEN    "aAqK_GD-4zTGxXazC2laXxsCqQNFfXgg"

// --- BACKEND ---
static const char* BACKEND_URL = "https://your-backend.railway.app/api/telemetry";

// --- PINS (MATCHING YOUR HARDWARE) ---
#define BUTTON_PIN      9      // Push Button
#define LED_PIN         8      // Onboard LED
#define GPS_RX_PIN      0      // Connects to GPS TX
#define GPS_TX_PIN      1      // Connects to GPS RX
#define BATTERY_PIN     4      // Connected to Voltage Divider

// --- SETTINGS ---
#define GPS_BAUD        9600
#define GPS_LOG_INTERVAL 1500
#define LONG_PRESS_TIME  3000

// --- BATTERY MATH (UPDATED FOR 5V SOURCE) ---
#define VOLTAGE_DIVIDER 2.0    
#define MAX_BAT_V       5.2    // JM: Changed from 4.2 to 5.2 for 5V source
#define MIN_BAT_V       3.5    // JM: Changed from 3.0 to 3.5 (safe cutoff)

// --- BLE ---
#define SERVICE_UUID           "172f3570-5645-4290-9519-046c64d85147"
#define CHAR_CREDENTIALS_UUID  "2388432a-360e-4363-8025-055171736417"

// --- MQTT CONFIG ---
// JM: Remove "tls://" prefix. The hostname is just the address.
#define MQTT_SERVER      "296f68459e704b318eb8bf075b9f3067.s1.eu.hivemq.cloud"
#define MQTT_PORT        8883   
#define MQTT_USER        "pawlar_mqtt"
#define MQTT_PASSWORD    "Pawlar123"

// --- MQTT TOPICS (Must match main.cpp) ---

// 1. PUBLISH (ESP32 sends data TO these)
#define TOPIC_GPS_PUB          "pawlar/collar/gps"
#define TOPIC_BATTERY_PUB      "pawlar/collar/battery/esp_to_app"

// 2. SUBSCRIBE (ESP32 listens FROM these)
#define TOPIC_BATTERY_SUB      "pawlar/collar/battery/app_to_esp"

#endif