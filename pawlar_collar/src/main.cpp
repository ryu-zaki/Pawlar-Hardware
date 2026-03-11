/**
 * @file main.cpp
 * @brief Pawlar Collar - WiFi Stable Mode (Cellular Logic Commented Out)
 */
#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <PubSubClient.h>
#include <WiFiClientSecure.h> 

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

void IRAM_ATTR isr() { btnPressed = true; }

// --- 🔋 BATTERY FUNCTION (10% Increments) ---
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
    percentage = (percentage / 10) * 10; 
    return percentage;
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
            String batPayload = "{\"id\": \"" + getUniqueDeviceID() + "\", \"bat\": " + String(batLevel) + "}";
            client.publish(TOPIC_BATTERY_SHARED, batPayload.c_str());
        }
    }
}

// --- 🔄 RECONNECT ---
void mqtt_reconnect() {
    if (WiFi.status() == WL_CONNECTED && !client.connected()) {
        String clientId = "PawlarCollar-" + getUniqueDeviceID();
        Serial.print("Connecting to HiveMQ...");
        if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
            Serial.println("✅ CONNECTED!");
            client.subscribe(TOPIC_BATTERY_SHARED); 
            client.subscribe(TOPIC_COMMANDS);
        } else {
            Serial.print("❌ Failed, rc=");
            Serial.println(client.state()); 
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    initStorage();
    Serial.println("\n🚀 Pawlar System Starting...");

    // 1. READ THE SAVED STATE FROM STORAGE 🚩
    pairingMode = isPairingRequested(); // <--- ADD THIS LINE

    // 🔘 CONFIGURE BUTTON & INTERRUPT
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), isr, FALLING);

    pinMode(LED_PIN, OUTPUT); 
    digitalWrite(LED_PIN, HIGH);

    // 2. Start BLE with the CORRECT mode
    initBLE(pairingMode); 

    // initGPS();
    initCellular();

    // 3. Network Config
    testWifiClient.setInsecure(); 
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(mqtt_callback);

    // If pairingMode is false, it means we are in BEACON mode
    if (!pairingMode) {
        String s = getSSID(); String p = getPass();
        if (s != "") connectToCloud(s, p); 
    }
}

void loop() {

    // 2. 🛰️ GPS & NETWORK LOGIC
    // readGPS(); 

    bool isWiFiAvailable = (WiFi.status() == WL_CONNECTED);

    if (isWiFiAvailable && !pairingMode) {
        if (!client.connected()) mqtt_reconnect();
        client.loop(); 

        if (millis() - lastSend > SEND_INTERVAL) {
            int bat = getBatteryPercentage();
            if (hasFix()) {
                String gpsPayload = "{\"id\": \"" + getUniqueDeviceID() + "\", \"lat\": " + String(getLat(), 6) + ", \"lng\": " + String(getLng(), 6) + ", \"sats\": " + String(getSatellites()) + ", \"status\": \"LOCKED\"}";
                client.publish(TOPIC_GPS_PUB, gpsPayload.c_str());
                Serial.println("📤 Sent GPS (WiFi): " + gpsPayload);
            } else {
                String scanPayload = "{\"id\": \"" + getUniqueDeviceID() + "\", \"status\": \"SCANNING\", \"sats\": " + String(getSatellites()) + "}";
                client.publish(TOPIC_GPS_PUB, scanPayload.c_str());
                Serial.println("🛰️ GPS Scanning");
            }
            lastSend = millis();
        }
    }
    else if (!pairingMode && !isWiFiAvailable) {
        // FAILOVER LOGIC
        static unsigned long lastCellUpdate = 0;
        if (millis() - lastCellUpdate > 60000) { 
            lastCellUpdate = millis();
            Serial.println("📶 WiFi Lost. Attempting 4G Failover...");
            sendCellularMQTT(getLat(), getLng(), getBatteryPercentage());
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
                delay(100); 
                yield(); // Prevents Watchdog reset

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