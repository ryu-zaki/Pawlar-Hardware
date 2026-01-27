#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// --- ⚙️ MOTOR PINS (L298N) ---
#define MOTOR_IN1       26
#define MOTOR_IN2       25
#define MOTOR_ENA       32 // PWM Pin for speed control

// --- 🔒 LOCK PINS (SG90 Servos) ---
#define SERVO_LEFT_PIN  13
#define SERVO_RIGHT_PIN 12

// --- 🛡️ SAFETY PIN (IR Break Beam) ---
#define IR_SENSOR_PIN   34 // Digital Input (Safety check)

// --- ☁️ MQTT CONFIG (HiveMQ Cloud) ---
// Must match your collar's credentials to talk to the same backend
#define MQTT_SERVER      "296f68459e704b318eb8bf075b9f3067.s1.eu.hivemq.cloud"
#define MQTT_PORT        8883   
#define MQTT_USER        "pawlar_mqtt"
#define MQTT_PASSWORD    "Pawlar123"

// --- 📧 MQTT TOPICS ---
#define TOPIC_DOOR_LOGS  "pawlar/door/logs"     // Door distance records
#define TOPIC_COMMANDS   "pawlar/collar/commands"

// --- 🔵 BLE SETTINGS ---
#define DOOR_SETUP_NAME  "PawlarDoor_Setup"
#define SERVICE_UUID     "172f3570-5645-4290-9519-046c64d85147" // Match Collar

#endif