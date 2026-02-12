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
    
    // ... Characteristic code remains the same ...
    BLECharacteristic *pId = pS->createCharacteristic(
        "beb5483e-36e1-4688-b7f5-ea07361b26a9", 
        BLECharacteristic::PROPERTY_READ
    );
    String idJson = "{\"dev_id\":\"" + getUniqueDeviceID() + "\",\"type\":\"collar\"}";
    pId->setValue(idJson.c_str());

    pS->start();

    // --- 📡 ADVERTISING OPTIMIZATION ---
    BLEAdvertising *pAdv = BLEDevice::getAdvertising();
    
    // 1. ADD THIS: Explicitly set the name in the advertisement
    // This is what the Door looks for in 'device.getName()'
    pAdv->addServiceUUID(SERVICE_UUID);
    pAdv->setScanResponse(true); 
    
    // 2. BOOST POWER: Ensure the signal is strong enough to hit -65dBm
    BLEDevice::setPower(ESP_PWR_LVL_P9); 

    // 3. SET INTERVALS: (Already good, but keep these)
    pAdv->setMinInterval(0x20); // 20ms
    pAdv->setMaxInterval(0x40); // 40ms
    
    // 4. IMPORTANT: Re-apply the name to the advertising data specifically
    BLEAdvertisementData advData;
    advData.setName(deviceName.c_str());
    advData.setFlags(0x06); // General Discoverable Mode
    pAdv->setAdvertisementData(advData);

    BLEDevice::startAdvertising();
    
    Serial.print(isPairing ? "🔵 PAIRING MODE: " : "✅ BEACON MODE: ");
    Serial.println(deviceName);
    Serial.println("🚀 Beacon Broadcasting at High Frequency");
}