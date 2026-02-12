#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// --- ⚙️ MOTOR PINS (L298N) ---
#define MOT_A_IN1 26
#define MOT_A_IN2 25
#define MOT_A_ENA 32
#define MOT_B_IN3 33
#define MOT_B_IN4 27
#define MOT_B_ENB 14

// --- 🔘 BUTTON PINS ---
#define BTN_UP 18
#define BTN_DOWN 19

// --- 👁️ SENSOR PINS ---
#define IR_INSIDE_PIN  13
#define IR_OUTSIDE_PIN 16 

#define I2C_SDA 22
#define I2C_SCL 21

// --- ☁️ MQTT CONFIG (HiveMQ Cloud) ---
#define MQTT_SERVER      "296f68459e704b318eb8bf075b9f3067.s1.eu.hivemq.cloud"
#define MQTT_PORT        8883   
#define MQTT_USER        "pawlar_mqtt"
#define MQTT_PASSWORD    "Pawlar123"

// --- 🔵 SHARED BLE UUIDS ---
#define SERVICE_UUID           "172f3570-5645-4290-9519-046c64d85147"
#define CHAR_CREDENTIALS_UUID  "2388432a-360e-4363-8025-055171736417"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// --- 📧 MQTT TOPICS ---
#define TOPIC_DOOR_LOGS  "pawlar/door/logs"     
#define TOPIC_COMMANDS   "pawlar/collar/commands"

#endif