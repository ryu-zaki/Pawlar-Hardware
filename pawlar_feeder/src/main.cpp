#include <Arduino.h>
#include "config.h"
#include "servo_manager.h"
#include "ultrasonic_manager.h"
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

unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 5000; // Check level every 5 seconds

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

    // 1. Manual dispensing
    if (digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("Button Pressed! Manual Dispense.");
        servoManager.dispense();
        publishFeederActivity("MANUAL_DISPENSE", 1.0);
        delay(500); // Debounce
    }

    // 2. Periodic level check
    if (millis() - lastUpdate >= UPDATE_INTERVAL) {
        lastUpdate = millis();
        
        FeederState state = ultrasonicManager.getState();
        float distance = ultrasonicManager.getDistance();
        
        Serial.print("Food Level: ");
        Serial.print(distance);
        Serial.print(" cm | State: ");
        Serial.println(ultrasonicManager.stateToString(state));
        
        if (state == STATE_LOW) {
            publishFeederActivity("FOOD_LOW", distance);
        }
    }
}
