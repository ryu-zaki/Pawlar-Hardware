#include "config.h"
#include "network_manager.h"
#include "storage_manager.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "cellular_manager.h"

extern WiFiClientSecure testWifiClient;
extern PubSubClient client;

void connectToCloud(String ssid, String pass) {
    if (ssid == "") return;
    
    Serial.print("📶 Connecting to WiFi: "); Serial.println(ssid);
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    int a = 0; 
    while(WiFi.status() != WL_CONNECTED && a < 20) { 
        delay(500); 
        Serial.print("."); 
        a++; 
    }
    
    if(WiFi.status() == WL_CONNECTED) { 
        Serial.println("\n✅ WiFi Connected!");
        testWifiClient.setInsecure(); 
        client.setServer(MQTT_SERVER, MQTT_PORT);
        client.setKeepAlive(15); 

        String macAddr = getMACAddress();
        String clientId = "Pawlar-" + macAddr;
        String lwtTopic = TOPIC_STATUS;
        String offlinePayload = "{\"device_id\": \"" + getUniqueDeviceID() + "\", \"message\": \"OFFLINE_UNEXPECTED\"}";

        String wifiStatusTopic = String(TOPIC_WIFI_PUB) + "/" + macAddr;

        if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD, lwtTopic.c_str(), 0, false, offlinePayload.c_str())) {
            Serial.println("✅ MQTT Connected instantly!");

            // 1. Send specific WiFi confirmation for the App
            String wifiPayload = "{\"device_id\": \"" + getUniqueDeviceID() + "\", \"isConnected\": true}";
            client.publish(wifiStatusTopic.c_str(), wifiPayload.c_str());
            Serial.println("📤 Sent WiFi Confirmation: " + wifiPayload);

            // 2. Send Online Notification
            publishNotification("Collar Online", "is now online.", "INFO");

            if (isNewlyRegistered()) {
                publishNotification("New Collar Registered", "is now registered to your account.", "INFO");
                setNewlyRegistered(false);
            }
            
            client.subscribe(TOPIC_BATTERY_SHARED);
        } else {
            Serial.println("❌ MQTT Connection Failed!");
            // sendWifiStatusCellular(false); // Disabled: Backend not ready
        }
    } else {
        Serial.println("\n❌ WiFi Connection Failed!");
        // sendWifiStatusCellular(false); // Disabled: Backend not ready
    }
}

void sendLocationData(float lat, float lng, int sats) {
    if (WiFi.status() != WL_CONNECTED) return;

    if (Blynk.connected()) {
        Blynk.virtualWrite(V0, lat);
        Blynk.virtualWrite(V1, lng);
        Blynk.virtualWrite(V2, sats);
    }

    HTTPClient http;
    http.begin(BACKEND_URL);
    http.addHeader("Content-Type", "application/json");
    
    JsonDocument doc; 
    doc["device_id"] = getUniqueDeviceID();
    doc["latitude"] = lat;
    doc["longitude"] = lng;
    doc["satellites"] = sats;
    
    String json;
    serializeJson(doc, json);
    int code = http.POST(json);
    http.end();
}

void publishNotification(String title, String description, String type, String trigger_id) {
    if (!client.connected()) return;

    JsonDocument doc;
    doc["device_id"] = getUniqueDeviceID();
    doc["device_type"] = "COLLAR";
    doc["title"] = title;
    doc["description"] = description;
    doc["type"] = type;
    if (trigger_id != "") {
        doc["trigger_id"] = trigger_id;
    }

    String payload;
    serializeJson(doc, payload);
    client.publish(TOPIC_NOTIFICATIONS, payload.c_str());
    Serial.println("📤 Published Notification: " + payload);
}