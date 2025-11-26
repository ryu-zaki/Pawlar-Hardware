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

    // 1. Send to Blynk (Prototype Visualization)
    if (Blynk.connected()) {
        Blynk.virtualWrite(V0, lat);
        Blynk.virtualWrite(V1, lng);
        Blynk.virtualWrite(V2, sats);
    }

    // 2. Send to PRODUCTION BACKEND (The "Contract")
    HTTPClient http;
    http.begin(BACKEND_URL); // URL defined in config.h
    http.addHeader("Content-Type", "application/json");
    
    StaticJsonDocument<256> doc;
    
    // --- THIS IS THE PAYLOAD STRUCTURE FOR THE DATABASE ---
    doc["device_id"]     = getUniqueDeviceID(); // String (VARCHAR)
    doc["latitude"]      = lat;                 // Float  (DECIMAL)
    doc["longitude"]     = lng;                 // Float  (DECIMAL)
    doc["satellites"]    = sats;                // Int    (INTEGER)
    doc["battery_level"] = 95;                  // Int    (SMALLINT) - Placeholder
    doc["status"]        = "active";            // String (VARCHAR)
    // ----------------------------------------------------
    
    String jsonBody;
    serializeJson(doc, jsonBody);

    int httpResponseCode = http.POST(jsonBody);
    
    if(httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("🚀 Sent to Backend: " + response);
    } else {
        Serial.printf("❌ Backend Error: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
}