#include <Arduino.h>
#include "config.h"
#include "servo_manager.h"
#include "ultrasonic_manager.h"
#include "loadcell_manager.h"
#include "storage_manager.h"
#include "network_manager.h"
#include "proximity_manager.h"
#include "ble_manager.h"
#include <esp_task_wdt.h>
// Global State
String authorizedCollarsCache = "";
TaskHandle_t BLETask;

// Manager Instances
ServoManager servoManager(SERVO_PIN);
UltrasonicManager ultrasonicManager(TRIG_PIN, ECHO_PIN);
LoadCellManager loadCellManager(HX711_DT_PIN, HX711_SCK_PIN);

unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 5000; // Check sensors every 5 seconds

// --- Core 0 Task: Bluetooth Scanning ---
void BLELoop(void * pvParameters) {
    Serial.print("🔵 BLE Task started on Core: ");
    Serial.println(xPortGetCoreID());

    esp_task_wdt_add(NULL);
    initProximityScan();

    for(;;) {
        esp_task_wdt_reset();
        scanForCollar();
        vTaskDelay(50 / portTICK_PERIOD_MS); 
    }
}

void setup() {
    Serial.begin(115200);
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    initStorage();
    servoManager.begin();
    ultrasonicManager.begin();
    loadCellManager.begin();
    
    String myId = getDeviceId();
    Serial.println("🆔 Device ID: " + myId);
    Serial.println("📡 MQTT Topic: pawlar/feeder/linked-collars/" + myId);
    
    authorizedCollarsCache = getAuthorizedCollarList();
    Serial.println("📋 Loaded Authorized Collars: " + authorizedCollarsCache);

    String ssid = getSSID();
    String pass = getPass();

    if (ssid == "") {
        Serial.println("⚠️ No WiFi saved. Entering BLE Provisioning Mode...");
        initBLEProvisioning();
    } else {
        if (connectToWiFi(ssid, pass)) {
            initNetwork();
            
            // Start BLE Scanning Task on Core 0
            xTaskCreatePinnedToCore(
                BLELoop,
                "BLE_Task",
                10000,
                NULL,
                1,
                &BLETask,
                0
            );
        } else {
            Serial.println("❌ WiFi failed to connect. Falling back to BLE Provisioning...");
            initBLEProvisioning();
        }
    }

    Serial.println("Pawlar Feeder Initialized.");
    Serial.println("Press button on GPIO 4 to dispense manually.");
}

void loop() {
    if (client.connected()) client.loop();

    // 1. Button Logic (Manual Dispense vs. Long-Press Tare)
    if (digitalRead(BUTTON_PIN) == LOW) {
        unsigned long pressStart = millis();
        bool isLongPress = false;
        
        while (digitalRead(BUTTON_PIN) == LOW) {
            if (millis() - pressStart > 2000) { // 2 seconds for tare
                isLongPress = true;
                break;
            }
            delay(10);
        }

        if (isLongPress) {
            Serial.println("⚖️ Button Long-Pressed! Taring Scale...");
            loadCellManager.tare();
            // Wait for release
            while(digitalRead(BUTTON_PIN) == LOW) delay(10);
        } else {
            float target = getGramsPerServing();
            Serial.println("Button Pressed! Manual Dispense (" + String(target) + "g target).");
            servoManager.dispenseWeight(target, loadCellManager);
            publishFeederActivity("MANUAL_DISPENSE", target);
        }
        delay(500); // Debounce
    }

    // 2. Periodic level check
    if (millis() - lastUpdate >= UPDATE_INTERVAL) {
        lastUpdate = millis();
        
        FeederState state = ultrasonicManager.getState();
        float distance = ultrasonicManager.getDistance();
        float weight = loadCellManager.getWeight(5);
        
        Serial.print("Food Level: ");
        Serial.print(distance);
        Serial.print(" cm | State: ");
        Serial.print(ultrasonicManager.stateToString(state));
        Serial.print(" | Weight: ");
        Serial.print(weight);
        Serial.println(" g");
        
        if (state == STATE_LOW) {
            publishFeederActivity("FOOD_LOW", distance);
        }
        
        // Publish weight to MQTT if needed
        if (weight >= 0) {
            publishFeederActivity("WEIGHT_CHECK", weight);
        }
    }
}
