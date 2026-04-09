#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 
#include "network_manager.h"
#include "storage_manager.h"
#include "config.h"
#include "loadcell_manager.h"
#include "servo_manager.h"

extern String authorizedCollarsCache; 
extern LoadCellManager loadCellManager;
extern ServoManager servoManager;
extern unsigned long lastDispenseTime;

WiFiClientSecure feederWifiClient;
PubSubClient client(feederWifiClient);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) message += (char)payload[i];
    String topicStr = String(topic);

    Serial.println("📩 MQTT Message [" + topicStr + "]: " + message);

    String myId = getDeviceId();
    String myLinkedCollarsTopic = "pawlar/feeder/linked-collars/" + myId;
    String mySyncTopic = "pawlar/feeder/" + myId + "/sync";
    String myCmdTopic = "pawlar/feeder/" + myId + "/cmd";

    // --- HANDLE FEEDER CONTROLS (App Manual Dispense) ---
    if (topicStr == TOPIC_FEEDER_CONTROLS) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        if (!error) {
            String command = doc["command"].as<String>();
            String deviceId = doc["device_id"].as<String>();
            bool confirmed = doc["confirmed"] | false;

            if (deviceId == myId && command == "FEED" && !confirmed) {
                float target = getGramsPerServing();
                Serial.println("📱 App Command: FEEDing " + String(target) + "g...");
                
                if (servoManager.dispenseWithBlockage(target, loadCellManager)) {
                    publishFeederConfirmation("FEED", target, true);
                    lastDispenseTime = millis(); // Reset 3-hour interval
                }
            }
        }
        return;
    }

    // --- HANDLE GRAMS CONFIGURATION ---
    if (topicStr == TOPIC_FEEDER_GRAMS) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        if (!error) {
            String deviceId = doc["device_id"].as<String>();
            if (deviceId == myId) {
                float amount = doc["amount"].as<float>();
                saveGramsPerServing(amount);
                Serial.println("⚙️ Grams Config: Updated Serving Size to " + String(amount) + "g");
                publishFeederActivity("GRAMS_UPDATED", amount);
            }
        }
        return;
    }

    if (topicStr == myCmdTopic) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        if (!error) {
            String command = doc["command"].as<String>();
            if (command == "tare") {
                Serial.println("⚖️ Remote Command: Taring Scale...");
                loadCellManager.tare();
                publishFeederActivity("CMD_TARE", 0);
            } else if (command == "dispense") {
                float target = doc["amount"] | getGramsPerServing();
                Serial.println("🍖 Remote Command: Dispensing " + String(target) + "g...");
                if (servoManager.dispenseWithBlockage(target, loadCellManager)) {
                    publishFeederActivity("CMD_DISPENSE", target);
                    lastDispenseTime = millis(); // 🚩 Reset the 3-hour interval for collars
                }
            } else if (command == "config") {
                if (doc["grams_per_serving"].is<float>()) {
                    float g = doc["grams_per_serving"].as<float>();
                    saveGramsPerServing(g);
                    Serial.println("⚙️ Updated grams_per_serving to: " + String(g));
                    publishFeederActivity("CONFIG_UPDATED", g);
                }
            }
        }
        return;
    }

    if (topicStr == myLinkedCollarsTopic || topicStr == mySyncTopic || topicStr == "pawlar/feeder/sync") {
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
                        if (v["device_id"].is<JsonVariant>()) id = v["device_id"].as<String>();
                    } else {
                        id = v.as<String>();
                    }
                    if (id != "") {
                        if (collarList != "") collarList += "|";
                        collarList += id;
                    }
                }
            } else if (doc.is<JsonObject>() && doc["device_id"].is<JsonVariant>()) {
                validMessage = true;
                collarList = doc["device_id"].as<String>();
            }

            if (validMessage) {
                saveAuthorizedCollar(collarList);
                authorizedCollarsCache = collarList; 
                Serial.println("💾 NVS & Cache Updated! Authorized Collars: " + collarList);
                publishNotification("Collar Linked to Feeder", "is now linked to a collar.", "INFO");
            }
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

void initNetwork() {
    feederWifiClient.setInsecure(); 
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(mqttCallback); 
    client.setKeepAlive(15); 
    
    String feederIdentity = getDeviceId(); 
    String lwtTopic = TOPIC_FEEDER_STATUS;
    String offlinePayload = "{\"device_id\": \"" + feederIdentity + "\", \"message\": \"OFFLINE_UNEXPECTED\"}";
    String onlineStatusPayload = "{\"device_id\": \"" + feederIdentity + "\", \"message\": \"ONLINE\"}";

    String wifiStatusTopic = "pawlar/feeder/wifi/" + feederIdentity;
    String linkedCollarsTopic = "pawlar/feeder/linked-collars/" + feederIdentity;

    int retryCount = 0;
    while (!client.connected() && retryCount < 3) {
        if (client.connect(feederIdentity.c_str(), MQTT_USER, MQTT_PASSWORD, lwtTopic.c_str(), 0, false, offlinePayload.c_str())) {
            Serial.println("✅ HiveMQ Connected!");
            
            // --- PUBLISH ONLINE STATUS (Same format as LWT) ---
            client.publish(lwtTopic.c_str(), onlineStatusPayload.c_str(), true); // Retained

            // Subscriptions
            client.subscribe(linkedCollarsTopic.c_str()); 
            client.subscribe("pawlar/feeder/sync");
            String cmdTopic = "pawlar/feeder/" + feederIdentity + "/cmd";
            client.subscribe(cmdTopic.c_str());
            Serial.println("📡 Subscribed to: " + cmdTopic);

            client.subscribe(TOPIC_FEEDER_GRAMS);
            Serial.println("📡 Subscribed to: " + String(TOPIC_FEEDER_GRAMS));

            client.subscribe(TOPIC_FEEDER_CONTROLS);
            Serial.println("📡 Subscribed to: " + String(TOPIC_FEEDER_CONTROLS));

            String onlinePayload = "{\"device_id\": \"" + feederIdentity + "\", \"isConnected\": true}";
            client.publish(wifiStatusTopic.c_str(), onlinePayload.c_str());

            // Send Online Notification
            publishNotification("Feeder Online", "is now online.", "INFO");

            if (isNewlyRegistered()) {
                publishNotification("New Feeder Registered", "is now registered.", "INFO");
                setNewlyRegistered(false);
            }

        } else {
            delay(1000); 
            retryCount++;
        }
    }
}

void publishFeederActivity(String event, float data) {
    String deviceId = getDeviceId();
    String payload = "{\"device\":\"" + deviceId + "\", \"event\":\"" + event + "\", \"data\":" + String(data, 2) + "}";
    if (client.connected()) {
        client.publish("pawlar/feeder/activity", payload.c_str());
    }
}

void publishFeederConfirmation(String command, float amount, bool confirmed) {
    String deviceId = getDeviceId();
    String confStr = confirmed ? "true" : "false";
    String payload = "{\"command\": \"" + command + "\", \"device_id\": \"" + deviceId + "\", \"amount\": " + String(amount, 1) + ", \"confirmed\": " + confStr + "}";
    
    if (client.connected()) {
        client.publish(TOPIC_FEEDER_CONTROLS, payload.c_str());
        Serial.println("📤 Published Confirmation: " + payload);
    }
}

void publishNotification(String title, String description, String type, String trigger_id) {
    if (!client.connected()) return;

    JsonDocument doc;
    doc["device_id"] = getDeviceId();
    doc["device_type"] = "FEEDER";
    doc["title"] = title;
    doc["description"] = description;
    doc["type"] = type;
    if (trigger_id != "") {
        doc["trigger_id"] = trigger_id;
    }

    String payload;
    serializeJson(doc, payload);
    client.publish(TOPIC_FEEDER_NOTIFICATIONS, payload.c_str());
    Serial.println("📤 Published Notification: " + payload);
}
