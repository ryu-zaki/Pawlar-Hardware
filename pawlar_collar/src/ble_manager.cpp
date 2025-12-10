#include "ble_manager.h"
#include "config.h"
#include "storage_manager.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

class PairingCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pC) {
        std::string val = pC->getValue();
        if (val.length() > 0) {
            StaticJsonDocument<256> doc;
            deserializeJson(doc, val.c_str());
            const char* s = doc["ssid"]; const char* p = doc["password"];
            if (s && p) {
                saveWiFiCreds(s, p);
                setPairingRequest(false);
                Serial.println("💾 Saved! Rebooting...");
                delay(1000); ESP.restart();
            }
        }
    }
};

void initBLE(bool isPairing) {
    String deviceName = getUniqueDeviceID();

    if (isPairing) {
        deviceName += " Setup"; 
    }

    BLEDevice::init(deviceName.c_str());

    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pS = pServer->createService(SERVICE_UUID);
    
    BLECharacteristic *pId = pS->createCharacteristic("beb5483e-36e1-4688-b7f5-ea07361b26a9", BLECharacteristic::PROPERTY_READ);
    String idJson = "{\"dev_id\":\"" + getUniqueDeviceID() + "\",\"type\":\"collar\"}";
    pId->setValue(idJson.c_str());

    if (isPairing) {
        BLECharacteristic *pCreds = pS->createCharacteristic(CHAR_CREDENTIALS_UUID, BLECharacteristic::PROPERTY_WRITE);
        pCreds->setCallbacks(new PairingCallbacks());
        digitalWrite(LED_PIN, LOW);
        Serial.print("🔵 PAIRING MODE: ");
        Serial.println(deviceName);
    } else {
        Serial.print("✅ BEACON MODE: ");
        Serial.println(deviceName);
    }
    pS->start();
    BLEAdvertising *pAdv = BLEDevice::getAdvertising();
    pAdv->addServiceUUID(SERVICE_UUID);
    pAdv->setScanResponse(true);
    BLEDevice::startAdvertising();
}