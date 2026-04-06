#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 
#include "network_manager.h"
#include "storage_manager.h"
#include "battery_manager.h"
#include "proximity_manager.h"
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

    String myId = getDeviceId();
    String myLinkedCollarsTopic = "pawlar/door/linked-collars/" + myId;
    String mySyncTopic = "pawlar/door/" + myId + "/sync";

    // --- HANDLE DOOR CONTROLS ---
    if (topicStr == TOPIC_DOOR_CONTROLS) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        if (!error) {
            String deviceId = doc["device_id"].as<String>();
            bool confirmed = doc["confirmed"].as<bool>();
            String state = doc["state"].as<String>();

            if (deviceId == myId && !confirmed) {
                handleRemoteCommand(state);
            }
        }
        return;
    }

    // --- HANDLE COLLAR SYNC (Either Topic) ---
    if (topicStr == myLinkedCollarsTopic || (topicStr == mySyncTopic && message.indexOf("device_id") != -1)) {
        JsonDocument doc; 
        DeserializationError error = deserializeJson(doc, message);

        if (!error) {
            String collarList = "";
            bool validMessage = false;

            if (doc.is<JsonArray>()) {
                validMessage = true; 
                for (JsonVariant v : doc.as<JsonArray>()) {
                    String id = "";
                    if (v.is<JsonObject>()) {
                        if (v["device_id"].is<JsonVariant>()) {
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
            else if (doc.is<JsonObject>() && doc["device_id"].is<JsonVariant>()) {
                validMessage = true;
                collarList = doc["device_id"].as<String>();
            }

            if (validMessage) {
                saveAuthorizedCollar(collarList);
                
                // 🚩 Update the global RAM cache instantly!
                authorizedCollarsCache = collarList; 
                
                Serial.println("💾 NVS & Cache Updated! Authorized Collars: " + collarList);
                publishDoorActivity("AUTH_SYNC_COMPLETE", 0.0);
                publishNotification("Collar Linked to Door", "is now linked with a new collar.", "INFO");
            } else {
                Serial.println("⚠️ JSON received, but no valid collar data was found.");
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
    client.setKeepAlive(15); 
    
    String doorIdentity = getDeviceId(); 
    String lwtTopic = TOPIC_DOOR_STATUS;
    String offlinePayload = "{\"device_id\": \"" + doorIdentity + "\", \"message\": \"OFFLINE_UNEXPECTED\"}";

    String wifiStatusTopic = "pawlar/door/wifi/" + doorIdentity;
    String linkedCollarsTopic = "pawlar/door/linked-collars/" + doorIdentity;

    Serial.println("☁️ Connecting to HiveMQ...");
    
    int retryCount = 0;
    while (!client.connected() && retryCount < 3) {
        Serial.printf("Attempt %d as %s\n", retryCount + 1, doorIdentity.c_str());
        
        if (client.connect(doorIdentity.c_str(), MQTT_USER, MQTT_PASSWORD, lwtTopic.c_str(), 0, false, offlinePayload.c_str())) {
            Serial.println("✅ HiveMQ Connected!");
            
            // --- SUBSCRIBE TO RELEVANT TOPICS ---
            client.subscribe(linkedCollarsTopic.c_str()); 
            Serial.println("👂 Subscribed to: " + linkedCollarsTopic);

            String syncTopic = "pawlar/door/" + doorIdentity + "/sync";
            client.subscribe(syncTopic.c_str());
            Serial.println("👂 Subscribed to: " + syncTopic);

            client.subscribe("pawlar/door/sync"); // General sync topic
            Serial.println("👂 Subscribed to: pawlar/door/sync");

            client.subscribe(TOPIC_DOOR_CONTROLS);
            Serial.println("👂 Subscribed to: " + String(TOPIC_DOOR_CONTROLS));

            String onlinePayload = "{\"device_id\": \"" + doorIdentity + "\", \"isConnected\": true}";
            client.publish(wifiStatusTopic.c_str(), onlinePayload.c_str());
            Serial.println("📤 Published Status: " + onlinePayload);

            // Send Online Notification
            publishNotification("Door Online", "is now online.", "INFO");

            if (isNewlyRegistered()) {
                publishNotification("New Door Registered", "is now registered.", "INFO");
                setNewlyRegistered(false);
            }

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

void publishDoorConfirmation(String state, bool confirmed) {
    String deviceId = getDeviceId();
    String confStr = confirmed ? "true" : "false";
    String payload = "{\"state\": \"" + state + "\", \"device_id\": \"" + deviceId + "\", \"confirmed\": " + confStr + "}";
    
    if (client.connected()) {
        client.publish(TOPIC_DOOR_CONTROLS, payload.c_str());
        Serial.println("📤 Published Confirmation: " + payload);
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

void publishNotification(String title, String description, String type, String trigger_id) {
    if (!client.connected()) return;

    JsonDocument doc;
    doc["device_id"] = getDeviceId();
    doc["device_type"] = "DOOR";
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