#include <Arduino.h>
#include "config.h"
#include "storage_manager.h"   // Fixes initStorage, getSSID, etc.
#include "network_manager.h"   // Fixes initNetwork, logTriggerEvent
#include "proximity_manager.h" // Fixes scanForCollar
#include "ble_manager.h"       // Fixes initBLEProvisioning

void setup() {
    Serial.begin(115200);
    initStorage(); // Initialize NVS

    String ssid = getSSID();
    String pass = getPass();

    if (ssid == "") {
        Serial.println("🔵 No WiFi found. Entering App Pairing Mode...");
        initBLEProvisioning(); // App connects here to send WiFi details
    } else {
        Serial.println("🌐 Connecting to Home WiFi: " + ssid);
        if (connectToWiFi(ssid, pass)) {
            initNetwork();       // Start MQTT
            initProximityScan(); // Start listening for Collar
        }
    }
}

void loop() {
    // Check for WiFi/MQTT connection status here
    
    // Constantly scan for the collar
    scanForCollar(); 
    
    delay(500); // Small delay to prevent overheating
}