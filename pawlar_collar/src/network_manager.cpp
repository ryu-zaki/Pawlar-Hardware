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

        String macAddr = getMACAddress();
        String clientId = "Pawlar-" + macAddr;
        if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
            Serial.println("✅ MQTT Connected instantly!");

            // 1. Send specific WiFi confirmation for the App
            String wifiPayload = "{\"device_id\": \"" + macAddr + "\", \"isConnected\": true}";
            String wifiTopic = String(TOPIC_WIFI_PUB) + "/" + macAddr;
            client.publish(wifiTopic.c_str(), wifiPayload.c_str());
            Serial.println("📤 Sent WiFi Confirmation: " + wifiPayload);

            // 2. Send general status
            String statusPayload = "{";
            statusPayload += "\"id\": \"" + getUniqueDeviceID() + "\","; 
            statusPayload += "\"status\": \"ONLINE\",";
            statusPayload += "\"ip\": \"" + WiFi.localIP().toString() + "\"";
            statusPayload += "}";

            client.publish(TOPIC_STATUS, statusPayload.c_str()); 
            client.subscribe(TOPIC_BATTERY_SHARED);
        } else {
            Serial.println("❌ MQTT Connection Failed!");
            sendWifiStatusCellular(false);
        }
    } else {
        Serial.println("\n❌ WiFi Connection Failed!");
        // Notify app via Cellular if WiFi failed
        sendWifiStatusCellular(false);
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