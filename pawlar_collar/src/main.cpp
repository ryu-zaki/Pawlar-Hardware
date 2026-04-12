/**
 * @file main.cpp
 * @brief Pawlar Collar - WiFi Stable Mode (Cellular Logic Commented Out)
 */
#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <PubSubClient.h>
#include <WiFiClientSecure.h> 
#include <ArduinoJson.h>

// --- INCLUDES ---
#include "config.h"
#include "storage_manager.h"
#include "gps_manager.h"
#include "network_manager.h"
#include "ble_manager.h"
#include "cellular_manager.h"

// The Serial handle for the A7670C (Keep for Serial Bridge)
// extern SoftwareSerial swSerial;
// #define CELL_PORT swSerial

WiFiClientSecure testWifiClient;
PubSubClient client(testWifiClient);

const unsigned long SEND_INTERVAL = 2000; 

// --- GLOBAL VARIABLES ---
bool pairingMode = false;
volatile bool btnPressed = false;
unsigned long lastSend = 0; 
bool lowBatteryNotified = false;
int lastReportedPercent = -1; // 🚩 Track last sent value to avoid spam

void IRAM_ATTR isr() { btnPressed = true; }

// --- 💡 CONNECTION LED LOGIC ---
void updateConnectionLED() {
    // ON only if connected to WiFi AND the Cloud (MQTT)
    if (WiFi.status() == WL_CONNECTED && client.connected()) {
        digitalWrite(LED_CONN_PIN, HIGH);
    } else {
        digitalWrite(LED_CONN_PIN, LOW);
    }
}

// --- 🔋 BATTERY FUNCTION (Quantized 25/50/75/100) ---
int getBatteryPercentage() {
    long sum = 0;
    int samples = 50; 
    for(int i=0; i < samples; i++) {
        sum += analogRead(BATTERY_PIN);
        delay(2);
    }
    float averageAdc = sum / (float)samples;
    float voltage = (averageAdc / 4095.0) * 3.3 * VOLTAGE_DIVIDER;
    int percentage = map(voltage * 100, MIN_BAT_V * 100, MAX_BAT_V * 100, 0, 100);
    percentage = constrain(percentage, 0, 100);
    
    // 🚩 QUANTIZATION LOGIC
    if (percentage <= 25) return 25;
    if (percentage <= 50) return 50;
    if (percentage <= 75) return 75;
    return 100;
}

// --- 🎧 MQTT CALLBACK ---
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) message += (char)payload[i];
    message.trim(); 
    
    String topicStr = String(topic);
    Serial.println("\n📬 Msg: [" + topicStr + "] " + message);

    if (topicStr == TOPIC_BATTERY_SHARED) {
        if (message == "GET_BATTERY" || message == "REFRESH") {
            int batLevel = getBatteryPercentage();
            String batPayload = "[{\"device_id\": \"" + getUniqueDeviceID() + "\", \"battery_level\": " + String(batLevel) + "}]";
            client.publish(TOPIC_BATTERY_SHARED, batPayload.c_str());
            lastReportedPercent = batLevel; // Sync last reported
        }
    }
}

// --- 🔄 RECONNECT ---
void mqtt_reconnect() {
    if (WiFi.status() == WL_CONNECTED && !client.connected()) {
        String clientId = "PawlarCollar-" + getUniqueDeviceID();
        Serial.print("Connecting to HiveMQ...");

        String lwtTopic = TOPIC_STATUS;
        String deviceId = getUniqueDeviceID();
        
        // Format matching backend StatusPayloadDto
        String offlinePayload = "{\"device_id\": \"" + deviceId + "\", \"message\": \"OFFLINE_UNEXPECTED\"}";
        String onlinePayload = "{\"device_id\": \"" + deviceId + "\", \"message\": \"ONLINE\"}";

        if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD, lwtTopic.c_str(), 0, false, offlinePayload.c_str())) {
            Serial.println("✅ CONNECTED!");
            
            // 1. Tell backend we are ONLINE (Retained)
            client.publish(lwtTopic.c_str(), onlinePayload.c_str(), true);

            // 2. Send App Notifications
            publishNotification("Collar Online", "is now online.", "INFO");
            
            if (isNewlyRegistered()) {
                publishNotification("New Collar Registered", "is now registered to your account.", "INFO");
                setNewlyRegistered(false);
            }

            client.subscribe(TOPIC_BATTERY_SHARED); 
            client.subscribe(TOPIC_COMMANDS);
        } else {
            Serial.print("❌ Failed, rc=");
            Serial.println(client.state()); 
            
            // SMART DELAY: Wait 5 seconds, but keep reading the GPS
            unsigned long waitStart = millis();
            while(millis() - waitStart < 5000) {
                readGPS();
                delay(10);
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    initStorage();
    Serial.println("\n🚀 Pawlar System Starting...");

    // 1. READ THE SAVED STATE FROM STORAGE
    pairingMode = isPairingRequested(); 
    String s = getSSID();

    // 🚩 AUTO-PAIRING: If no WiFi is saved, force Pairing Mode
    if (s == "") {
        Serial.println("⚠️ No WiFi saved. Entering BLE Pairing Mode automatically...");
        pairingMode = true;
    }

    // 🔘 CONFIGURE BUTTON & INTERRUPT
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), isr, FALLING);

    pinMode(LED_PIN, OUTPUT); 
    digitalWrite(LED_PIN, HIGH);
    
    pinMode(LED_CONN_PIN, OUTPUT);
    digitalWrite(LED_CONN_PIN, LOW); // Start OFF

    // 2. Start BLE with the CORRECT mode
    initBLE(pairingMode); 

    initGPS();
    initCellular();

    // --- 🔍 BOOT DIAGNOSTICS ---
    Serial.println("\n--- 🛠️ System State ---");
    Serial.println("Pairing Mode: " + String(pairingMode ? "ON (BLE Active)" : "OFF (Network Active)"));
    Serial.println("Saved SSID: " + (s == "" ? "[EMPTY]" : s));
    Serial.println("Device ID: " + getUniqueDeviceID());
    Serial.println("----------------------\n");

    // 3. Network Config
    testWifiClient.setInsecure(); 
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(mqtt_callback);

    if (!pairingMode && s != "") {
        connectToCloud(s, getPass()); 
    } 
}

void loop() {
    updateConnectionLED(); // 🚩 Update Green LED status

    // 2. 🛰️ GPS & NETWORK LOGIC
    readGPS(); 

    // Diagnostic: Check if GPS is actually decoding data
    static unsigned long lastGpsCheck = 0;
    if (millis() - lastGpsCheck > 10000) {
        lastGpsCheck = millis();
        if (!isGpsCommuncating()) {
            Serial.println("⚠️ GPS ALERT: No data being decoded. Check baud rate/pins!");
        } else {
            Serial.printf("🛰️ GPS STATUS: Decoding OK. Sats visible: %d\n", getSatellites());
        }
    }

    bool isWiFiAvailable = (WiFi.status() == WL_CONNECTED);

    if (isWiFiAvailable && !pairingMode) {
        if (!client.connected()) mqtt_reconnect();
        client.loop(); 

        if (millis() - lastSend > SEND_INTERVAL) {
            int bat = getBatteryPercentage();
            
            // 🚩 Only publish to App if the quantized percentage has changed
            if (bat != lastReportedPercent) {
                String batPayload = "[{\"device_id\": \"" + getUniqueDeviceID() + "\", \"battery_level\": " + String(bat) + "}]";
                client.publish(TOPIC_BATTERY_SHARED, batPayload.c_str());
                lastReportedPercent = bat;
                Serial.printf("📤 Published Quantized Battery: %d%%\n", bat);
            }

            if (bat <= 25 && !lowBatteryNotified) {
                publishNotification("Battery Low", "battery is low. Please charge it soon.", "WARNING");
                lowBatteryNotified = true;
            } else if (bat > 25) {
                lowBatteryNotified = false;
            }

            if (hasFix()) {
                // Corrected payload format for map display
                String gpsPayload = "{\"device_id\": \"" + getUniqueDeviceID() + "\", \"coords\": {\"lat\": " + String(getLat(), 6) + ", \"long\": " + String(getLng(), 6) + "}}";
                client.publish(TOPIC_GPS_PUB, gpsPayload.c_str());
                Serial.println("📤 Sent GPS (WiFi): " + gpsPayload);
            } else {
                Serial.println("🛰️ GPS Scanning (WiFi Active)");
            }
            lastSend = millis();
        }
    }
    else if (!pairingMode && !isWiFiAvailable) {
        // --- FAILOVER LOGIC (4G Persistent) ---
        static unsigned long lastCellUpdate = 0;

        if (millis() - lastCellUpdate > 30000) { // Send every 30 seconds
            lastCellUpdate = millis();
            
            if (hasFix()) {
                sendCellularMQTT(getLat(), getLng(), getBatteryPercentage(), getSatellites(), "LOCKED");
            } else {
                Serial.printf("🛰️ 4G FAILOVER: GPS Scanning... (Sats: %d)\n", getSatellites());
                sendCellularMQTT(0.0, 0.0, getBatteryPercentage(), getSatellites(), "SCANNING");
            }
        }
    }

    // 3. 🔘 BUTTON LOGIC
    if (btnPressed) {
        delay(50); // Simple debounce
        if (digitalRead(BUTTON_PIN) == LOW) {
            unsigned long start = millis();
            bool handled = false;

            Serial.println("🔘 Button Hold Detected...");

            while (digitalRead(BUTTON_PIN) == LOW) {
                digitalWrite(LED_PIN, !digitalRead(LED_PIN)); 
                
                unsigned long dStart = millis();
                while(millis() - dStart < 100) { 
                    readGPS(); 
                    delay(5); 
                }
                yield();
                unsigned long holdTime = millis() - start;

                if (holdTime > 10000) {
                    Serial.println("♻️ 10s Hold: Wiping WiFi & Restarting...");
                    WiFi.disconnect(true, true); 
                    digitalWrite(LED_PIN, LOW); 
                    delay(1000); 
                    ESP.restart(); 
                    handled = true; 
                    break; 
                }
            }

            unsigned long finalHold = millis() - start;
            if (!handled && (finalHold > LONG_PRESS_TIME)) {
                Serial.println("🔵 3s Hold: Toggling Pairing Mode...");
                setPairingRequest(!pairingMode); 
                delay(500); 
                ESP.restart();
            }
        }
        btnPressed = false;
        digitalWrite(LED_PIN, HIGH); 
    }
}