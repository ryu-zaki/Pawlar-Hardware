#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "network_manager.h"
#include "storage_manager.h"
#include "battery_manager.h"
#include "config.h"

WiFiClientSecure doorWifiClient;
PubSubClient client(doorWifiClient);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) message += (char)payload[i];
    String topicStr = String(topic);
    
    Serial.println("📩 MQTT Message [" + topicStr + "]: " + message);

    // FIX: Check if the topic ends with /sync to handle dynamic door IDs
    if (topicStr.endsWith("/sync") || topicStr == "pawlar/door/sync") {
        if (message == "GET_BATTERY") {
            reportBatteryHealth();
        } else {
            // 1. Save the Collar ID received from Postman/Backend
            saveAuthorizedCollar(message); 
            
            // 2. VERIFY
            String verifiedList = getAuthorizedCollarList();
            Serial.println("💾 NVS Updated! Authorized: " + verifiedList);
            
            // 3. Feedback to Backend
            publishDoorActivity("AUTH_SYNC_COMPLETE", 0.0);
        }
    } 
}

bool connectToWiFi(String ssid, String pass) {
    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.print("🌐 Connecting to WiFi: " + ssid);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500); Serial.print("."); attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi Connected!");
        return true;
    }
    return false;
}

void requestCollarSync() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        
        // 1. Ensure this IP matches your computer running the backend
        String serverPath = "http://192.168.0.110:3001/doors/all-collars"; 
        
        http.begin(serverPath);
        http.addHeader("Content-Type", "application/json");

        String httpRequestData = "{\"user_id\":\"" + getUserId() + "\"}";

        Serial.println("📡 Syncing with DB...");
        int httpResponseCode = http.POST(httpRequestData);
        
        if (httpResponseCode == 200) {
            String response = http.getString();
            Serial.println("📥 Payload: " + response); // Debug print to see what backend sent

            JsonDocument doc; 
            DeserializationError error = deserializeJson(doc, response);

            if (!error) {
                // 🔄 CHANGE 1: Handle Root Array
                // Prisma returns [ {data}, {data} ], NOT { "collars": [...] }
                if (doc.is<JsonArray>()) {
                    JsonArray collars = doc.as<JsonArray>();
                    String collarList = "";

                    for (JsonObject collar : collars) {
                        // 🔄 CHANGE 2: Check for 'device_id' (Standard) or 'pet_id'
                        // Matches the "COLLAR_XXXX" format from your Collar Code
                        String fetchedId = "";
                        
                        if (collar.containsKey("device_id")) {
                            fetchedId = collar["device_id"].as<String>();
                        } else if (collar.containsKey("pet_id")) {
                            fetchedId = collar["pet_id"].as<String>();
                        }

                        if (fetchedId.length() > 0) {
                            if (collarList != "") collarList += "|";
                            collarList += fetchedId;
                        }
                    }

                    if (collarList != "") {
                        saveAuthorizedCollar(collarList);
                        Serial.println("💾 DB Sync Success: " + collarList);
                    } else {
                        Serial.println("⚠️ Connected, but user has no collars.");
                    }
                } else {
                    Serial.println("❌ Error: Backend did not return an Array []");
                }
            } else {
                Serial.print("❌ JSON Error: ");
                Serial.println(error.c_str());
            }
        } else {
            Serial.println("❌ DB Sync Failed. Code: " + String(httpResponseCode));
        }
        http.end();
    }
}

void initNetwork() {
    doorWifiClient.setInsecure(); 
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(mqttCallback); 
    
    String doorIdentity = getUniqueDoorID(); 
    String syncTopic = "pawlar/door/" + doorIdentity + "/sync";

    Serial.println("☁️ Connecting to HiveMQ...");
    
    int retryCount = 0;
    while (!client.connected() && retryCount < 3) {
        Serial.printf("Attempt %d as %s\n", retryCount + 1, doorIdentity.c_str());
        
        if (client.connect(doorIdentity.c_str(), MQTT_USER, MQTT_PASSWORD)) {
            Serial.println("✅ HiveMQ Connected!");
            
            // 🔥 SUCCESS: Subscribe now that we are connected
            client.subscribe(syncTopic.c_str()); 
            client.subscribe("pawlar/door/sync"); // Fallback global topic
            
            Serial.println("👂 Subscribed to: " + syncTopic);

            String statePayload = "{\"device_id\": \"" + doorIdentity + "\", \"connected\": true}";
            client.publish("pawlar/door/wifi", statePayload.c_str());

            requestCollarSync();
        } else {
            Serial.printf("❌ Failed (rc=%d). Retrying...\n", client.state());
            delay(1000); 
            retryCount++;
        }
    }

    if (!client.connected()) {
        Serial.println("⚠️ MQTT Offline. Manual mode only.");
    }
}

void publishDoorActivity(String event, double distance) {
    String deviceId = getDeviceId();
    String payload = "{\"device\":\"" + deviceId + "\", \"event\":\"" + event + "\", \"dist\":" + String(distance, 2) + "}";
    if (client.connected()) {
        client.publish("pawlar/door/activity", payload.c_str());
    }
}

void logTriggerEvent(int rssi, double distance) {
    publishDoorActivity("PROXIMITY_OPEN", distance);
    Serial.println("📤 Activity Log Published.");
}

void publishBatteryHealth(float voltage, float current, int percentage) {
    String deviceId = getDeviceId();
    String payload = "{\"device\":\"" + deviceId + "\", \"type\":\"HEALTH\", \"voltage\":" + String(voltage, 2) + 
                     ", \"battery\":" + String(percentage) + "}";
    
    if (client.connected()) {
        client.publish("pawlar/door/activity", payload.c_str());
    }
}