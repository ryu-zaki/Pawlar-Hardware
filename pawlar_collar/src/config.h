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
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_CREDENTIALS_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#endif