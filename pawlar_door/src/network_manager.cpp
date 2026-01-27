#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "network_manager.h"
#include "config.h"

WiFiClientSecure doorWifiClient;
PubSubClient client(doorWifiClient);

// --- New Function to handle WiFi Connection ---
bool connectToWiFi(String ssid, String pass) {
    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.print("🌐 Connecting to WiFi: " + ssid);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi Connected! IP: " + WiFi.localIP().toString());
        return true;
    } else {
        Serial.println("\n❌ WiFi Connection Failed.");
        return false;
    }
}

void initNetwork() {
    doorWifiClient.setInsecure(); 
    client.setServer(MQTT_SERVER, MQTT_PORT);
    
    while (!client.connected()) {
        Serial.print("☁️ Connecting to HiveMQ...");
        // Use a unique client ID for the door
        if (client.connect("PawlarDoor_01", MQTT_USER, MQTT_PASSWORD)) {
            Serial.println("connected!");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void logTriggerEvent(int rssi, double distance) {
    String payload = "{\"id\": \"PET_DOOR_01\", \"event\": \"PROXIMITY_OPEN\", \"rssi\": " + String(rssi) + ", \"dist_m\": " + String(distance, 2) + "}";
    
    if (client.connected()) {
        client.publish(TOPIC_DOOR_LOGS, payload.c_str());
        Serial.println("📤 Door Log Sent to Backend: " + payload);
    }
}