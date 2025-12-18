/**
 * @file main.cpp
 * @brief Pawlar Collar - Full System (GPS Sender + Status Receiver)
 */
#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <PubSubClient.h>

// --- INCLUDES ---
#include "config.h"
#include "storage_manager.h"
#include "gps_manager.h"
#include "network_manager.h"
#include "ble_manager.h"

WiFiClient testWifiClient;
PubSubClient client(testWifiClient);

// --- CONFIGURATION ---
const unsigned long GPS_SEND_INTERVAL = 15000; 

// --- GLOBAL VARIABLES ---
bool pairingMode = false;
volatile bool btnPressed = false;
unsigned long lastGPSSend = 0; 

void IRAM_ATTR isr() { btnPressed = true; }

// --- 🎧 THE RECEIVER LOGIC (Callback) ---
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) message += (char)payload[i];
    String topicStr = String(topic);

    Serial.println("\n📬 INCOMING MESSAGE!");
    Serial.print("   Topic: "); Serial.println(topicStr);
    Serial.print("   Payload: "); Serial.println(message);

    // --- CHECK WHICH TOPIC TRIGGERED THIS ---
    
    // 1. Handle STATUS Updates (collar/status)
    if (topicStr == TOPIC_STATUS) {
        Serial.print("ℹ️ STATUS UPDATE: ");
        
        if (message == "OPEN_DOOR") {
            Serial.println("Door is Opening! 🚪");
            // Blink LED 3 times to simulate action
            for(int i=0; i<3; i++) { digitalWrite(LED_PIN, LOW); delay(200); digitalWrite(LED_PIN, HIGH); delay(200); }
        }
        else if (message == "FEED_PET") {
            Serial.println("Feeder is Active! 🍖");
        }
        else {
            Serial.println("Status: " + message);
        }
    }

    // 2. Handle COMMANDS (pawlar/collar/commands) - Keep existing logic just in case
    else if (topicStr == TOPIC_COMMANDS) { 
         if (message == "RESTART") ESP.restart();
    }
}

// --- 🔄 RECONNECT & SUBSCRIBE ---
void mqtt_reconnect() {
    if (WiFi.status() == WL_CONNECTED && !client.connected()) {
        Serial.print("🔌 Connecting to MQTT...");
        String clientId = "PawlarCollar-" + getUniqueDeviceID();
        
        if (client.connect(clientId.c_str())) {
            Serial.println("Connected!");
            
            // --- SUBSCRIBE TO TOPICS HERE ---
            client.subscribe(TOPIC_STATUS);    // <--- LISTENING TO 'collar/status'
            client.subscribe(TOPIC_COMMANDS);  // <--- LISTENING TO 'pawlar/collar/commands'
            
            Serial.println("🎧 Subscribed to: " + String(TOPIC_STATUS));
            Serial.println("🎧 Subscribed to: " + String(TOPIC_COMMANDS));

        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" (retrying in 5s)");
            delay(1000); 
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n🚀 Pawlar System Starting...");

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, HIGH);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), isr, FALLING);

    initGPS();
    Serial.println("🆔 ID: " + getUniqueDeviceID());

    pairingMode = isPairingRequested();
    initBLE(pairingMode);

    if (!pairingMode) {
        String s = getSSID(); String p = getPass();
        if (s != "") {
             Serial.println("Connecting to WiFi...");
             connectToCloud(s, p); 
        }
    }

    // --- SETUP MQTT ---
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(mqtt_callback);
}

void loop() {
    // 1. Keep GPS Alive
    readGPS();

    // 2. Maintain MQTT Connection
    if (!pairingMode && WiFi.status() == WL_CONNECTED) {
        if (!client.connected()) {
            mqtt_reconnect();
        }
        client.loop(); // Checks for incoming messages

        // 3. 📤 SENDER: Send GPS Data
        if (millis() - lastGPSSend > GPS_SEND_INTERVAL) {
            String payload = "{";
            payload += "\"id\": \"" + getUniqueDeviceID() + "\","; 
            payload += "\"lat\": " + String(getLat(), 6) + ",";
            payload += "\"lng\": " + String(getLng(), 6);
            payload += "}";

            client.publish(TOPIC_GPS, payload.c_str());
            Serial.print("📤 Sent GPS: "); Serial.println(payload);
            lastGPSSend = millis();
        }
    }

    // 4. Button Logic
    if (btnPressed) {
        delay(50);
        if (digitalRead(BUTTON_PIN) == LOW) {
            unsigned long start = millis();
            while (digitalRead(BUTTON_PIN) == LOW) {
                digitalWrite(LED_PIN, !digitalRead(LED_PIN)); delay(100);
            }
            if (millis() - start > LONG_PRESS_TIME) {
                setPairingRequest(!pairingMode);
                delay(500); ESP.restart();
            }
        }
        btnPressed = false;
    }
}