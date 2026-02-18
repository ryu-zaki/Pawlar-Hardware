#include "config.h"
#include "ble_manager.h"
#include "storage_manager.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// Callback to handle incoming WiFi credentials remains the same
class PairingCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pC) {
        std::string val = pC->getValue();
        if (val.length() > 0) {
            JsonDocument doc; 
            DeserializationError error = deserializeJson(doc, val.c_str());
            if (!error) {
                const char* s = doc["ssid"]; 
                const char* p = doc["password"];
                if (s && p) {
                    saveWiFiCreds(s, p);
                    setPairingRequest(false);
                    Serial.println("💾 WiFi Credentials Saved! Rebooting...");
                    delay(1000); 
                    ESP.restart();
                }
            }
        }
    }
};

void initBLE(bool isPairing) {
    String deviceName = getUniqueDeviceID(); // COLLAR_XXXXXXXX

    if (isPairing) {
        deviceName += " Setup"; 
    }

    BLEDevice::init(deviceName.c_str());

    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pS = pServer->createService(SERVICE_UUID);
    
    // --- 1. The ID Characteristic (Read Only - Existing code) ---
    BLECharacteristic *pId = pS->createCharacteristic(
        "beb5483e-36e1-4688-b7f5-ea07361b26a9", 
        BLECharacteristic::PROPERTY_READ
    );
    String idJson = "{\"dev_id\":\"" + getUniqueDeviceID() + "\",\"type\":\"collar\"}";
    pId->setValue(idJson.c_str());

    // --- 2. 🚩 NEW: The WiFi Credentials Characteristic (Write Only) ---
    // This MUST match the Door so the App can use the same code!
    BLECharacteristic *pWifi = pS->createCharacteristic(
        CHAR_CREDENTIALS_UUID, // Uses 2388432a-360e-4363-8025-055171736417 from config.h
        BLECharacteristic::PROPERTY_WRITE | 
        BLECharacteristic::PROPERTY_WRITE_NR
    );
    // Attach the callback so it actually parses the JSON when the App sends it!
    pWifi->setCallbacks(new PairingCallbacks());

    pS->start();

    // --- 📡 ADVERTISING OPTIMIZATION ---
    BLEAdvertising *pAdv = BLEDevice::getAdvertising();
    pAdv->addServiceUUID(SERVICE_UUID);
    pAdv->setScanResponse(true); 
    
    BLEDevice::setPower(ESP_PWR_LVL_P9); 

    pAdv->setMinInterval(0x20); // 20ms
    pAdv->setMaxInterval(0x40); // 40ms
    
    BLEAdvertisementData advData;
    advData.setName(deviceName.c_str());
    advData.setFlags(0x06); // General Discoverable Mode
    pAdv->setAdvertisementData(advData);

    BLEDevice::startAdvertising();
    
    Serial.print(isPairing ? "🔵 PAIRING MODE: " : "✅ BEACON MODE: ");
    Serial.println(deviceName);
    Serial.println("🚀 Beacon Broadcasting at High Frequency");
}