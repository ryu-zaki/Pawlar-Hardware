#include <Arduino.h>
#include "config.h"
#include "storage_manager.h"
#include "network_manager.h"
#include "proximity_manager.h"
#include "ble_manager.h"
#include "battery_manager.h" 
#include "safety_manager.h"

// --- Global State ---
bool isMoving = false;
TaskHandle_t BLETask; // Handle for the background task on Core 0

// --- Function Prototypes ---
void stopMotors();
void moveUp();
void moveDown();
void handleManualActivityLog(String event);

// --- Core 0 Task: Bluetooth Scanning ---
// This function runs independently on the second processor core.
void BLELoop(void * pvParameters) {
    Serial.print("🔵 BLE Task started on Core: ");
    Serial.println(xPortGetCoreID());

    initProximityScan();

    for(;;) {
        scanForCollar();
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

// --- Manual Activity Logging ---
void handleManualActivityLog(String event) {
    static unsigned long lastPub = 0;
    if (client.connected() && (millis() - lastPub > 2000)) {
        publishDoorActivity(event, 0.0);
        lastPub = millis();
    }
}

// --- Motor Control Functions ---
void stopMotors() {
    analogWrite(MOT_A_ENA, 0);
    analogWrite(MOT_B_ENB, 0);
}

void moveUp() {
    Serial.println("⬆️ Logic: Moving Up...");
    digitalWrite(MOT_A_IN1, HIGH); digitalWrite(MOT_A_IN2, LOW);
    digitalWrite(MOT_B_IN3, HIGH); digitalWrite(MOT_B_IN4, LOW);
    analogWrite(MOT_A_ENA, 255);
    analogWrite(MOT_B_ENB, 255);
}

void moveDown() {
    // 🛡️ ACTIVE SAFETY CHECK: 
    // If either sensor is blocked (LOW), refuse to move down.
    if (digitalRead(IR_INSIDE_PIN) == LOW || digitalRead(IR_OUTSIDE_PIN) == LOW) {
        Serial.println("🚫 SAFETY STOP: Obstacle detected in path. Motor inhibited.");
        stopMotors();
        isMoving = false; // Reset state so the loop doesn't fight the sensor
        return;
    }

    Serial.println("⬇️ Logic: Moving Down...");
    digitalWrite(MOT_A_IN1, LOW); digitalWrite(MOT_A_IN2, HIGH);
    digitalWrite(MOT_B_IN3, LOW); digitalWrite(MOT_B_IN4, HIGH);
    analogWrite(MOT_A_ENA, 255);
    analogWrite(MOT_B_ENB, 255);
}

void setup() {
    Serial.begin(115200);
    Serial.println("--- Pawlar Door: DUAL-CORE STABILITY MODE ---");

    // Initialize Pins
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(MOT_A_IN1, OUTPUT); pinMode(MOT_A_IN2, OUTPUT); pinMode(MOT_A_ENA, OUTPUT);
    pinMode(MOT_B_IN3, OUTPUT); pinMode(MOT_B_IN4, OUTPUT); pinMode(MOT_B_ENB, OUTPUT);

    // Initialize Managers
    initStorage();
    initBatteryMonitor();
    initSafetySensors();

    String ssid = getSSID();
    String pass = getPass();

    if (ssid == "") {
        initBLEProvisioning();
    } else {
        if (connectToWiFi(ssid, pass)) {
            initNetwork(); // Setup MQTT and HTTP Sync
            
            // --- Create Core 0 Task ---
            // This offloads the heavy BLE scanning to the other CPU core.
            xTaskCreatePinnedToCore(
                BLELoop,        /* Function to implement the task */
                "BLE_Task",     /* Name of the task */
                10000,          /* Stack size in words */
                NULL,           /* Task input parameter */
                1,              /* Priority of the task */
                &BLETask,       /* Task handle */
                0               /* Core ID (0) */
            );
        }
    }
    Serial.println("🚀 System Online. Core 1 handling Buttons/Motors.");
}

// ... (Keep includes and definitions the same)

void loop() {
    if (client.connected()) client.loop();
    checkIRActivity();

    bool btnUp = (digitalRead(BTN_UP) == LOW);
    bool btnDown = (digitalRead(BTN_DOWN) == LOW);

    // 1. High Priority: Manual Button Reading
    if (btnUp || btnDown) {
        isMoving = true; // Mark as moving so system knows motors are active
        if (btnUp) {
            moveUp();
            handleManualActivityLog("MANUAL_UP");
        }
        else if (btnDown) {
            moveDown();
            handleManualActivityLog("MANUAL_DOWN");
        }
        return;
    }

    // 2. Idle Logic
    // If NO buttons are pressed, and the system thinks it's moving, stop it.
    // The automated control is handled on Core 0. This is a manual override stop.
    if (!btnUp && !btnDown) {
        if (isMoving) {
            Serial.println("🛑 Manual Override: Stopping Motors.");
            stopMotors();
            isMoving = false;
        }
    }
}