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
extern unsigned long lastDispenseTime;
bool isRegisteredCached = false;
volatile bool isDispensing = false;

// Manager Instances
ServoManager servoManager(SERVO_PIN, BOWL_SERVO_PIN);
UltrasonicManager ultrasonicManager(TRIG_PIN, ECHO_PIN);
LoadCellManager loadCellManager(HX711_DT_PIN, HX711_SCK_PIN);

unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 1000; // Check sensors every 1 second

// --- RGB LED Logic ---
void setLED(bool r, bool g, bool b) {
    digitalWrite(LED_RED, r ? HIGH : LOW);
    digitalWrite(LED_GREEN, g ? HIGH : LOW);
    digitalWrite(LED_BLUE, b ? HIGH : LOW);
}

void updateLEDState() {
    static unsigned long lastBlink = 0;
    static bool blinkState = false;

    // 🚩 1. Dispensing State (BLUE) - Priority 1
    if (isDispensing) {
        setLED(false, false, true);
        return;
    }

    // 🚩 2. Not Registered/No WiFi saved -> SOLID RED
    if (!isRegisteredCached) {
        setLED(true, false, false);
        return;
    }

    // 🚩 3. Connecting (WiFi connecting or MQTT connecting) -> BLINK GREEN
    if (WiFi.status() != WL_CONNECTED || !client.connected()) {
        if (millis() - lastBlink > 500) {
            lastBlink = millis();
            blinkState = !blinkState;
            setLED(false, blinkState, false);
        }
    } 
    // 🚩 4. Fully Connected -> SOLID GREEN
    else {
        setLED(false, true, false);
    }
}

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
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);

    // Initial LED state
    setLED(true, false, false); // Default to RED until logic takes over

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
    isRegisteredCached = (ssid != ""); // 🚩 Set the cache for the LED logic

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
    updateLEDState();
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
            // Independently trigger the managed cycle (Always allowed)
            float target = getGramsPerServing();
            Serial.println("Button Pressed! Starting manual managed cycle (" + String(target) + "g).");
            
            isDispensing = true;
            if (servoManager.dispenseWithBlockage(target, loadCellManager)) {
                publishFeederActivity("MANUAL_DISPENSE", target);
                lastDispenseTime = millis(); // 🚩 Reset the 3-hour interval for collars
            }
            isDispensing = false;
        }
        delay(500); // Debounce
    }

    // 2. Periodic level check
    if (millis() - lastUpdate >= UPDATE_INTERVAL) {
        lastUpdate = millis();
        
        FeederState state = ultrasonicManager.getState();
        float distance = ultrasonicManager.getDistance();
        float weight = loadCellManager.getWeight(2); // Fewer samples for faster update
        
        Serial.print("Food Level: ");
        Serial.print(distance);
        Serial.print(" cm | State: ");
        Serial.print(ultrasonicManager.stateToString(state));
        Serial.print(" | Weight: ");
        Serial.print(weight);
        Serial.println(" g");
        
        if (state == STATE_LOW || state == STATE_EMPTY) {
            publishFeederActivity("FOOD_LOW", distance);
            static unsigned long lastStockAlert = 0;
            if (millis() - lastStockAlert > 3600000) { // Notify every hour
                String msg = "food stock is running low.";
                publishNotification("Food Stock Low", msg, "CRITICAL");
                lastStockAlert = millis();
            }
        }
        
        // Publish weight to MQTT if needed
        if (weight >= 0) {
            publishFeederActivity("WEIGHT_CHECK", weight);
        }
    }
}
