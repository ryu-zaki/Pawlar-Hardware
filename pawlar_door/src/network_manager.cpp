#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 
#include "network_manager.h"
#include "storage_manager.h"
#include "battery_manager.h"
#include "config.h"

// 🚩 This allows network_manager to see the variable from main.cpp
extern String authorizedCollarsCache; 

WiFiClientSecure doorWifiClient;
PubSubClient client(doorWifiClient);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) message += (char)payload[i];
    String topicStr = String(topic);
    
    Serial.println("📩 MQTT Message [" + topicStr + "]: " + message);

    String myLinkedCollarsTopic = "pawlar/door/linked-collars/" + getDeviceId();

    if (topicStr == myLinkedCollarsTopic) {
        JsonDocument doc; 
        DeserializationError error = deserializeJson(doc, message);

        if (!error) {
            String collarList = "";

            if (doc.is<JsonArray>()) {
                for (JsonVariant v : doc.as<JsonArray>()) {
                    String id = "";
                    if (v.is<JsonObject>()) {
                        if (v.containsKey("device_id")) {
                            id = v["device_id"].as<String>();
                        }
                    } else {
                        id = v.as<String>();
                    }

                    if (id != "") {
                        if (collarList != "") collarList += "|";
                        collarList += id;
                    }
                }
            } 
            else if (doc.is<JsonObject>() && doc.containsKey("device_id")) {
                collarList = doc["device_id"].as<String>();
            }

            if (collarList != "") {
                saveAuthorizedCollar(collarList);
                
                // 🚩 Update the global RAM cache instantly!
                authorizedCollarsCache = collarList; 
                
                Serial.println("💾 NVS & Cache Updated! Authorized Collars: " + collarList);
                publishDoorActivity("AUTH_SYNC_COMPLETE", 0.0);
            } else {
                Serial.println("⚠️ JSON received, but no 'device_id' was found.");
            }
        } else {
            Serial.print("❌ JSON Parse Error: ");
            Serial.println(error.c_str());
        }
    } 
    else if (message == "GET_BATTERY") {
        reportBatteryHealth();
    }
}

bool connectToWiFi(String ssid, String pass) {
    WiFi.disconnect(true); // 🚩 Ensure clean state
    delay(100); 

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

void initNetwork() {
    doorWifiClient.setInsecure(); 
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(mqttCallback); 
    
    String doorIdentity = getDeviceId(); 
    String wifiStatusTopic = "pawlar/door/wifi/" + doorIdentity;
    String linkedCollarsTopic = "pawlar/door/linked-collars/" + doorIdentity;
    String offlinePayload = "{\"device_id\": \"" + doorIdentity + "\", \"isConnected\": false}";

    Serial.println("☁️ Connecting to HiveMQ...");
    
    int retryCount = 0;
    while (!client.connected() && retryCount < 3) {
        Serial.printf("Attempt %d as %s\n", retryCount + 1, doorIdentity.c_str());
        
        if (client.connect(doorIdentity.c_str(), MQTT_USER, MQTT_PASSWORD, wifiStatusTopic.c_str(), 0, false, offlinePayload.c_str())) {
            Serial.println("✅ HiveMQ Connected!");
            
            client.subscribe(linkedCollarsTopic.c_str()); 
            Serial.println("👂 Subscribed to: " + linkedCollarsTopic);

            String onlinePayload = "{\"device_id\": \"" + doorIdentity + "\", \"isConnected\": true}";
            client.publish(wifiStatusTopic.c_str(), onlinePayload.c_str());
            Serial.println("📤 Published Status: " + onlinePayload);

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