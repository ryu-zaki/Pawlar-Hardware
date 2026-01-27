/**
 * @file main.cpp
 * @brief Pawlar Collar - 10% Battery Steps + GPS + HiveMQ
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
    
    // Formula: (ADC / Max_ADC) * Ref_Voltage * Divider_Factor (2.0 for 100k+100k)
    float voltage = (averageAdc / 4095.0) * 3.3 * VOLTAGE_DIVIDER;
    
    // Mapping 3.3V (0%) to 4.2V (100%)
    int percentage = map(voltage * 100, MIN_BAT_V * 100, MAX_BAT_V * 100, 0, 100);
    percentage = constrain(percentage, 0, 100);

    // ⚡ SNAP TO NEAREST 10% ⚡
    percentage = (percentage / 10) * 10; 

    return percentage;
}

// --- 🎧 MQTT CALLBACK ---
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) message += (char)payload[i];
    message.trim(); // Removes hidden spaces
    
    String topicStr = String(topic);
    Serial.println("\n📬 Msg: [" + topicStr + "] " + message);

    // Using the shared topic defined in your config.h
    if (topicStr == TOPIC_BATTERY_SHARED) {
        if (message == "GET_BATTERY" || message == "REFRESH") {
            Serial.println("🔋 Request Received! Reading Battery...");
            
            int batLevel = getBatteryPercentage();
            String batPayload = "{\"id\": \"" + getUniqueDeviceID() + "\", \"bat\": " + String(batLevel) + "}";
            
            client.publish(TOPIC_BATTERY_SHARED, batPayload.c_str());
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
            // Subscribe to the shared battery topic
            client.subscribe(TOPIC_BATTERY_SHARED); 
            client.subscribe(TOPIC_COMMANDS);
            Serial.println("👂 Subscribed to: " + String(TOPIC_BATTERY_SHARED));
        } else {
            Serial.print("❌ Failed, rc=");
            Serial.print(client.state()); 
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
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

    testWifiClient.setInsecure(); 
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(mqtt_callback);
}

void loop() {
    readGPS(); 

    if (!pairingMode && WiFi.status() == WL_CONNECTED) {
        if (!client.connected()) mqtt_reconnect();
        client.loop(); 

        if (millis() - lastSend > SEND_INTERVAL) {
            double lat = getLat();
            double lng = getLng();

            String gpsPayload = "{\"id\": \"" + getUniqueDeviceID() + "\", \"lat\": " + String(lat, 6) + ", \"lng\": " + String(lng, 6) + ", \"sats\": " + String(getSatellites()) + "}";

            if (client.connected()) {
                client.publish(TOPIC_GPS_PUB, gpsPayload.c_str());
                Serial.println("📤 Sent GPS: " + gpsPayload);
            }
            lastSend = millis();
        }
    }

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
        digitalWrite(LED_PIN, HIGH);
    }
}