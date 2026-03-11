#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 
#include "network_manager.h"
#include "storage_manager.h"
#include "config.h"

extern String authorizedCollarsCache; 

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
    
    String feederIdentity = getDeviceId(); 
    String statusTopic = "pawlar/feeder/wifi/" + feederIdentity;
    String linkedCollarsTopic = "pawlar/feeder/linked-collars/" + feederIdentity;

    int retryCount = 0;
    while (!client.connected() && retryCount < 3) {
        if (client.connect(feederIdentity.c_str(), MQTT_USER, MQTT_PASSWORD)) {
            Serial.println("✅ HiveMQ Connected!");
            client.subscribe(linkedCollarsTopic.c_str()); 
            client.subscribe("pawlar/feeder/sync");
            String onlinePayload = "{\"device_id\": \"" + feederIdentity + "\", \"isConnected\": true}";
            client.publish(statusTopic.c_str(), onlinePayload.c_str());
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
