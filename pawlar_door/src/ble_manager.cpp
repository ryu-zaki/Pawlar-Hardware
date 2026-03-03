#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h> // 🚩 Added ArduinoJson to parse the app's payload
#include "ble_manager.h"
#include "storage_manager.h" 
#include "network_manager.h"
#include "config.h"

class ProvisioningCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            String input = String(value.c_str());
            Serial.println("📥 WiFi Credentials received: " + input);

            // 🚩 1. Parse the incoming JSON from the app
            JsonDocument doc; 
            DeserializationError error = deserializeJson(doc, input);

            // 🚩 2. Check if parsing was successful
            if (!error) {
                const char* s = doc["ssid"]; 
                const char* p = doc["password"];

                // 🚩 3. Verify both keys exist
                if (s && p) {
                    String ssid = String(s);
                    String pass = String(p);

                    Serial.println("🔄 Testing WiFi Connection before saving...");
                    if (connectToWiFi(ssid, pass)) {
                        // Save to NVS ONLY IF successful
                        saveCredentials(ssid, pass); 
                        Serial.println("✅ WiFi Verified & Saved. Rebooting to establish cloud connection...");
                        delay(2000);
                        ESP.restart(); 
                    } else {
                        Serial.println("❌ WiFi Connection Failed. Credentials NOT saved.");
                        Serial.println("📢 Please check your SSID/Password and try again via the app.");
                    }
                } else {
                    Serial.println("❌ Error: JSON missing 'ssid' or 'password' keys.");
                }
            } else {
                Serial.print("❌ Error: Invalid JSON format. Reason: ");
                Serial.println(error.c_str());
            }
        }
    }
};

void initBLEProvisioning() {
    // Unique ID based setup name
    String doorSetupName = "DOOR_" + getUniqueDoorID() + " Setup";
    
    BLEDevice::init(doorSetupName.c_str()); 
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);

    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                             CHAR_CREDENTIALS_UUID,
                                             BLECharacteristic::PROPERTY_WRITE | 
                                             BLECharacteristic::PROPERTY_WRITE_NR
                                           );

    pCharacteristic->setCallbacks(new ProvisioningCallbacks());
    pService->start(); 

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    
    // Ensure name is visible in scan
    BLEAdvertisementData advData;
    advData.setName(doorSetupName.c_str());
    advData.setCompleteServices(BLEUUID(SERVICE_UUID));
    pAdvertising->setAdvertisementData(advData);
    
    pAdvertising->start();
    
    Serial.println("📡 BLE Setup Portal Active: " + doorSetupName);
}