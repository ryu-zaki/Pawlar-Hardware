#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// --- 📏 PROXIMITY SETTINGS ---
#define DISTANCE_RECOGNITION_LIMIT 15.0  // Logs collar presence at 3 meters
#define DISTANCE_UNLOCK_LIMIT      5.0  // Only unlocks at 1 meter

// --- ⚙️ MOTOR PINS (L298N) ---
#define MOTOR_IN1       26
#define MOTOR_IN2       25
#define MOTOR_ENA       32 

// --- 🔒 LOCK PINS (SG90 Servos) ---
//#define SERVO_LEFT_PIN  13
//#define SERVO_RIGHT_PIN 12

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

// --- 📧 MQTT TOPICS ---
#define TOPIC_DOOR_LOGS  "pawlar/door/logs"     
#define TOPIC_COMMANDS   "pawlar/collar/commands"

// --- 🔵 BLE SETTINGS ---
#define DOOR_SETUP_NAME     "PawlarDoor_Setup"
#define SERVICE_UUID        "172f3570-5645-4290-9519-046c64d85147"
// ADD THIS LINE:
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8" 

#endif