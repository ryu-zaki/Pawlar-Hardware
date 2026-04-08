#include "proximity_manager.h"
#include "storage_manager.h"
#include "servo_manager.h"
#include "network_manager.h"
#include <BLEDevice.h>
#include <Arduino.h>

extern String authorizedCollarsCache;
extern ServoManager servoManager;
extern LoadCellManager loadCellManager;

// --- CONFIG ---
const int RSSI_THRESHOLD_DISPENSE = -75; 
const unsigned long DISPENSE_COOLDOWN = 30000; // 30 seconds cooldown between auto-dispenses

unsigned long lastDispenseTime = 0;

void initProximityScan() {
    String feederName = "FEEDER_" + getDeviceId();
    BLEDevice::init(feederName.c_str());
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void scanForCollar() {
    String authList = authorizedCollarsCache;
    if (authList == "") return;

    BLEScan* pBLEScan = BLEDevice::getScan();
    BLEScanResults foundDevices = pBLEScan->start(1, false);

    bool authorizedCollarFound = false;
    int maxRssi = -100;
    String triggerId = "";

    for (int i = 0; i < foundDevices.getCount(); i++) {
        BLEAdvertisedDevice device = foundDevices.getDevice(i);
        String foundName = device.getName().c_str();
        String foundAddr = device.getAddress().toString().c_str();
        foundAddr.toUpperCase();

        bool isAuthorized = false;
        if (foundName.length() > 0 && authList.indexOf(foundName) != -1) isAuthorized = true;
        if (authList.indexOf(foundAddr) != -1) isAuthorized = true;

        if (isAuthorized) {
            if (device.getRSSI() > maxRssi) {
                maxRssi = device.getRSSI();
                triggerId = foundName.length() > 0 ? foundName : foundAddr;
            }
            authorizedCollarFound = true;
        }
    }
    pBLEScan->clearResults();

    if (authorizedCollarFound && maxRssi >= RSSI_THRESHOLD_DISPENSE) {
        if (millis() - lastDispenseTime > DISPENSE_COOLDOWN) {
            float target = getGramsPerServing();
            Serial.println("🐾 Authorized pet detected! Dispensing " + String(target) + "g food...");
            servoManager.dispenseWithBlockage(target, loadCellManager);
            publishFeederActivity("AUTO_DISPENSE", target);
            publishNotification("Food Dispensed", "successfully dispensed a meal.", "INFO", triggerId);
            lastDispenseTime = millis();
        } else {
            Serial.println("⏳ Pet still nearby, but cooldown active.");
        }
    }
}
