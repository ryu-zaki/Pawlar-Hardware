#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// --- ⚙️ PINS ---
#define BUTTON_PIN 4 
#define SERVO_PIN  13
#define TRIG_PIN   5
#define ECHO_PIN   18

// --- ☁️ MQTT CONFIG (HiveMQ Cloud) ---
#define MQTT_SERVER      "296f68459e704b318eb8bf075b9f3067.s1.eu.hivemq.cloud"
#define MQTT_PORT        8883   
#define MQTT_USER        "pawlar_mqtt"
#define MQTT_PASSWORD    "Pawlar123"

// --- 🔵 SHARED BLE UUIDS ---
#define SERVICE_UUID           "172f3570-5645-4290-9519-046c64d85147"
#define CHAR_CREDENTIALS_UUID  "2388432a-360e-4363-8025-055171736417"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#endif