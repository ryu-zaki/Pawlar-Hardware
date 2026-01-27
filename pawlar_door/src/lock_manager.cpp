#include "ble_manager.h"
#include "config.h"
#include "storage_manager.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

void initBLEProvisioning() {
    // Use the name defined in config.h
    BLEDevice::init(DOOR_SETUP_NAME);
    BLEServer *pServer = BLEDevice::createServer();
    
    // For now, we print to serial so you can test the "Zero-Wire" flow
    Serial.println("🔵 BLE Portal Active: " + String(DOOR_SETUP_NAME));
    Serial.println("📱 Waiting for App to provide WiFi and Collar ID...");
    
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  
    pAdvertising->start();
}

void unlockDoor() {
    Serial.println("🔓 [HARDWARE]: Servos Rotating to UNLOCK position...");
}