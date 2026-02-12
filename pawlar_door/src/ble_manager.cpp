#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "ble_manager.h"
#include "storage_manager.h" 
#include "config.h"

class ProvisioningCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            String input = String(value.c_str());
            Serial.println("📥 WiFi Credentials received: " + input);

            // App sends: ssid|password
            int splitIndex = input.indexOf('|');

            if (splitIndex != -1) {
                String ssid = input.substring(0, splitIndex);
                String pass = input.substring(splitIndex + 1);

                // Save to NVS
                saveCredentials(ssid, pass); 

                Serial.println("✅ WiFi Saved. Rebooting to establish cloud connection...");
                delay(2000);
                ESP.restart(); 
            } else {
                Serial.println("❌ Error: Invalid format received. Expected 'ssid|password'.");
            }
        }
    }
};

void initBLEProvisioning() {
    // Gamitin ang MAC address format para sa Setup Name
    String doorSetupName = getUniqueDoorID() + "_Setup";
    
    BLEDevice::init(doorSetupName.c_str()); 
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);

    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

    pCharacteristic->setCallbacks(new ProvisioningCallbacks());
    pService->start(); 

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    
    Serial.println("📡 BLE Setup Portal Active: " + doorSetupName);
}