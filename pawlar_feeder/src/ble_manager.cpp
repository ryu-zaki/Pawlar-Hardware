#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
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

            JsonDocument doc; 
            DeserializationError error = deserializeJson(doc, input);

            if (!error) {
                const char* s = doc["ssid"]; 
                const char* p = doc["password"];

                if (s && p) {
                    String ssid = String(s);
                    String pass = String(p);

                    Serial.println("🔄 Testing WiFi Connection...");
                    if (connectToWiFi(ssid, pass)) {
                        saveCredentials(ssid, pass); 
                        Serial.println("✅ WiFi Verified & Saved. Rebooting...");
                        delay(2000);
                        ESP.restart(); 
                    } else {
                        Serial.println("❌ WiFi Connection Failed.");
                    }
                }
            }
        }
    }
};

void initBLEProvisioning() {
    String feederSetupName = "FEEDER_" + getUniqueFeederID() + " Setup";
    
    BLEDevice::init(feederSetupName.c_str()); 
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
    
    BLEAdvertisementData advData;
    advData.setName(feederSetupName.c_str());
    advData.setCompleteServices(BLEUUID(SERVICE_UUID));
    pAdvertising->setAdvertisementData(advData);
    
    pAdvertising->start();
    
    Serial.println("📡 BLE Setup Portal Active: " + feederSetupName);
}
