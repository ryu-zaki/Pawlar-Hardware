/**
 * @file main.cpp
 * @brief Pawlar Collar - HiveMQ Secure + 5% Battery Steps
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

WiFiClientSecure testWifiClient;
PubSubClient client(testWifiClient);

// --- CONFIGURATION ---
const unsigned long SEND_INTERVAL = 2000; 

// --- GLOBAL VARIABLES ---
bool pairingMode = false;
volatile bool btnPressed = false;
unsigned long lastSend = 0; 

void IRAM_ATTR isr() { btnPressed = true; }

// --- 🔋 BATTERY FUNCTION (5% Increments) ---
int getBatteryPercentage() {
    long sum = 0;
    int samples = 50; 
    for(int i=0; i < samples; i++) {
        sum += analogRead(BATTERY_PIN);
        delay(2);
    }
    float averageAdc = sum / (float)samples;
    
    // Convert ADC to Voltage
    float voltage = (averageAdc / 4095.0) * 3.3 * VOLTAGE_DIVIDER;
    
    // Convert Voltage to Percentage (0-100)
    int percentage = map(voltage * 100, MIN_BAT_V * 100, MAX_BAT_V * 100, 0, 100);

    // Safety Clamping
    if (percentage > 100) percentage = 100;
    if (percentage < 0) percentage = 0;

    // ⚡ SNAP TO NEAREST 5% ⚡
    // This removes jitter (e.g., 83% becomes 80%, 88% becomes 85%)
    percentage = (percentage / 5) * 5; 

    return percentage;
}

// --- 🎧 MQTT CALLBACK ---
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) message += (char)payload[i];
    String topicStr = String(topic);
    Serial.println("\n📬 Msg: [" + topicStr + "] " + message);

    if (topicStr == TOPIC_BATTERY_SUB) {
        if (message == "GET_BATTERY" || message == "REFRESH") {
            Serial.println("🔋 Request Received! Reading Battery...");
            
            // Get the new stepped value (e.g., 80, 85, 90)
            int batLevel = getBatteryPercentage();
            
            String batPayload = "{";
            batPayload += "\"id\": \"" + getUniqueDeviceID() + "\","; 
            batPayload += "\"bat\": " + String(batLevel); 
            batPayload += "}";
            
            client.publish(TOPIC_BATTERY_PUB, batPayload.c_str());
            Serial.println("📤 Sent Response: " + batPayload);
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
            client.subscribe(TOPIC_BATTERY_SUB); 
            Serial.println("👂 Subscribed to: " + String(TOPIC_BATTERY_SUB));
        } else {
            Serial.print("❌ Failed, rc=");
            Serial.print(client.state()); 
            Serial.println(" retrying in 5s...");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n🚀 Pawlar System Starting...");

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, HIGH);
    
    analogSetAttenuation(ADC_11db); 
    pinMode(BATTERY_PIN, INPUT);

    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), isr, FALLING);

    initGPS();
    Serial.println("🆔 ID: " + getUniqueDeviceID());

    pairingMode = isPairingRequested();
    initBLE(pairingMode);

    if (!pairingMode) {
        String s = getSSID(); String p = getPass();
        if (s != "") connectToCloud(s, p);
    }

    // HiveMQ SSL Security
    testWifiClient.setInsecure(); 
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(mqtt_callback);
}

void loop() {
    readGPS(); 

    if (!pairingMode && WiFi.status() == WL_CONNECTED) {
        if (!client.connected()) mqtt_reconnect();
        client.loop(); 

        // GPS Sender
        if (millis() - lastSend > SEND_INTERVAL) {
            
            double lat = getLat();
            double lng = getLng();

            // Just for debugging GPS signal
            if (lat == 0.0 && lng == 0.0) {
                 // Serial.println("Searching for Satellites...");
            }

            String gpsPayload = "{";
            gpsPayload += "\"id\": \"" + getUniqueDeviceID() + "\","; 
            gpsPayload += "\"lat\": " + String(lat, 6) + ",";
            gpsPayload += "\"lng\": " + String(lng, 6);
            gpsPayload += "}";

            if (client.connected()) {
                client.publish(TOPIC_GPS_PUB, gpsPayload.c_str());
                Serial.println("📤 Sent GPS: " + gpsPayload);
            }
            lastSend = millis();
        }
    }

    // Button Logic
    if (btnPressed) {
        delay(50);
        if (digitalRead(BUTTON_PIN) == LOW) {
            unsigned long start = millis();
            bool handled = false;
            while (digitalRead(BUTTON_PIN) == LOW) {
                digitalWrite(LED_PIN, !digitalRead(LED_PIN)); delay(100);
                if (millis() - start > 10000) {
                    WiFi.disconnect(true, true); 
                    digitalWrite(LED_PIN, LOW); delay(2000); 
                    ESP.restart(); handled = true; break; 
                }
            }
            if (!handled && (millis() - start > LONG_PRESS_TIME)) {
                setPairingRequest(!pairingMode); delay(500); ESP.restart();
            }
        }
        btnPressed = false;
    }
}