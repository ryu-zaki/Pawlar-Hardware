#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// --- BLYNK (Required for network_manager.cpp) ---
#define BLYNK_TEMPLATE_ID   "TMPL6QodO-XJ1"
#define BLYNK_TEMPLATE_NAME "Pawlar Collar"
#define BLYNK_AUTH_TOKEN    "aAqK_GD-4zTGxXazC2laXxsCqQNFfXgg"

// --- 🌐 BACKEND ---
static const char* BACKEND_URL = "https://your-project-name.railway.app/api/telemetry";

// --- 📍 PINS (Matched to Sandwich Wiring Guide) ---
#define BUTTON_PIN      9      // Side Button (Pin 9 to GND)
#define LED_PIN         8      // Onboard System LED

// 🛰️ GPS (Using Pins 2 & 3 to avoid USB-Serial conflict)
#define GPS_RX_PIN      3     // GPS TX -> ESP32 RX (Pin 2)
#define GPS_TX_PIN      2      // GPS RX -> ESP32 TX (Pin 3)

// 🔋 Battery Health Sensor
#define BATTERY_PIN     4      // Analog Input from 100k Divider

// --- 📡 4G Module (A7670C) ---
#define GSM_RX_PIN      20     // 4G TX -> ESP32 RX
#define GSM_TX_PIN      21     // 4G RX -> ESP32 TX
#define GSM_BAUD        115200

// --- 📡 SETTINGS ---
#define GPS_BAUD         9600
#define GPS_LOG_INTERVAL 1500
#define LONG_PRESS_TIME  3000

// --- 🔋 BATTERY MATH ---
#define VOLTAGE_DIVIDER 2.0    // Ratio for two 100k resistors
#define MAX_BAT_V       4.2    // 100% health at 4.2V
#define MIN_BAT_V       3.3    // 0% health at 3.3V (Manual Unplug Level)

// --- ☁️ MQTT CONFIG (HiveMQ) ---
#define MQTT_SERVER      "296f68459e704b318eb8bf075b9f3067.s1.eu.hivemq.cloud"
#define MQTT_PORT        8883   
#define MQTT_USER        "pawlar_mqtt"
#define MQTT_PASSWORD    "Pawlar123"

// --- 🔵 BLE UUIDS ---
#define SERVICE_UUID           "172f3570-5645-4290-9519-046c64d85147"
#define CHAR_CREDENTIALS_UUID  "2388432a-360e-4363-8025-055171736417"

// --- 📧 MQTT TOPICS ---
#define TOPIC_GPS_PUB          "pawlar/collar/gps"
#define TOPIC_STATUS           "pawlar/collar/status"
#define TOPIC_BATTERY_SHARED   "pawlar/collar/battery"
#define TOPIC_COMMANDS         "pawlar/collar/commands"
#define TOPIC_WIFI_PUB         "pawlar/collar/wifi"

#endif