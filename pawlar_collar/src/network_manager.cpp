#include "config.h"            // MUST be the first include to define topics and URLs
#include "network_manager.h"
#include "storage_manager.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Externalize the clients defined in main.cpp
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
        Serial.print("🌐 IP Address: "); Serial.println(WiFi.localIP());

        // 1. Prepare MQTT Security
        testWifiClient.setInsecure(); 
        client.setServer(MQTT_SERVER, MQTT_PORT);

        // 2. Connect MQTT
        String clientId = "Pawlar-" + getUniqueDeviceID();
        if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
            Serial.println("✅ MQTT Connected instantly!");

            // 3. Prepare Online Status Payload
            String statusPayload = "{";
            statusPayload += "\"id\": \"" + getUniqueDeviceID() + "\","; 
            statusPayload += "\"status\": \"ONLINE\",";
            statusPayload += "\"ip\": \"" + WiFi.localIP().toString() + "\"";
            statusPayload += "}";

            // 4. Publish to Status Topic
            client.publish(TOPIC_STATUS, statusPayload.c_str()); 
            Serial.println("📤 Sent Status: " + statusPayload);
            
            // 5. Subscribe to the shared battery topic (FIXED NAME HERE)
            client.subscribe(TOPIC_BATTERY_SHARED);
            Serial.println("👂 Subscribed to: " + String(TOPIC_BATTERY_SHARED));

        } else {
            Serial.print("❌ MQTT Connect Failed, rc=");
            Serial.println(client.state());
        }
    } else {
        Serial.println("\n❌ WiFi Connect Failed.");
    }
}

void sendLocationData(float lat, float lng, int sats) {
    if (WiFi.status() != WL_CONNECTED) return;

    // 1. Update Blynk
    if (Blynk.connected()) {
        Blynk.virtualWrite(V0, lat);
        Blynk.virtualWrite(V1, lng);
        Blynk.virtualWrite(V2, sats);
    }

    // 2. Update Backend via HTTP
    HTTPClient http;
    http.begin(BACKEND_URL);
    http.addHeader("Content-Type", "application/json");
    
    StaticJsonDocument<200> doc;
    doc["device_id"] = getUniqueDeviceID();
    doc["latitude"] = lat;
    doc["longitude"] = lng;
    doc["satellites"] = sats;
    
    String json;
    serializeJson(doc, json);
    int code = http.POST(json);
    http.end();
}
