#include "network_manager.h"
#include "config.h"
#include "storage_manager.h"
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

void connectToCloud(String ssid, String pass) {
    if (ssid == "") return;
    Serial.print("📶 Connecting to WiFi: "); Serial.println(ssid);
    Blynk.config(BLYNK_AUTH_TOKEN);
    WiFi.begin(ssid.c_str(), pass.c_str());
    int a=0; while(WiFi.status()!=WL_CONNECTED && a<20){ delay(500); Serial.print("."); a++; }
    if(WiFi.status()==WL_CONNECTED) { Serial.println("\n✅ Connected!"); Blynk.connect(); }
}

void runCloud() { if(WiFi.status()==WL_CONNECTED) Blynk.run(); }

void sendLocationData(float lat, float lng, int sats) {
    if (WiFi.status() != WL_CONNECTED) return;

    // 1. Blynk
    if (Blynk.connected()) {
        Blynk.virtualWrite(V0, lat);
        Blynk.virtualWrite(V1, lng);
        Blynk.virtualWrite(V2, sats);
        Serial.println("☁️ Sent to Blynk");
    }

    // 2. Backend
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
    // if(code > 0) Serial.printf("🚀 Sent to Backend: %d\n", code);
    http.end();
}