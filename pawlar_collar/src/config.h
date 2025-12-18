#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// --- BLYNK ---
#define BLYNK_TEMPLATE_ID   "TMPL6QodO-XJ1"
#define BLYNK_TEMPLATE_NAME "Pawlar Collar"
#define BLYNK_AUTH_TOKEN    "aAqK_GD-4zTGxXazC2laXxsCqQNFfXgg"

// --- BACKEND ---
static const char* BACKEND_URL = "https://your-backend.railway.app/api/telemetry";

// --- PINS ---
#define BUTTON_PIN      9
#define LED_PIN         8
#define GPS_RX_PIN      0
#define GPS_TX_PIN      1

// --- SETTINGS ---
#define GPS_BAUD        9600
#define GPS_LOG_INTERVAL 1500
#define LONG_PRESS_TIME  3000

// --- BLE ---
#define SERVICE_UUID           "172f3570-5645-4290-9519-046c64d85147"
#define CHAR_CREDENTIALS_UUID  "2388432a-360e-4363-8025-055171736417"

// --- MQTT CONFIG (UPDATED) ---
#define MQTT_SERVER      "192.168.0.106"
#define MQTT_PORT        1883
#define MQTT_USER        ""            
#define MQTT_PASSWORD    ""             

#define TOPIC_COMMANDS   "pawlar/collar/commands" 
#define TOPIC_GPS        "pawlar/collar/gps" 
#define TOPIC_STATUS     "collar/status"

#endif