#include "proximity_manager.h"
#include "network_manager.h"
#include "storage_manager.h"
#include "lock_manager.h"
#include "safety_manager.h" 
#include <BLEDevice.h>
#include <Arduino.h>

extern bool isPathClear; 
extern bool isMoving; // 🔄 ADDED: Access to the main movement flag

void moveUp();
void moveDown();
void stopMotors();

const int RSSI_THRESHOLD_OPEN = -65; 
bool doorIsOpen = false;              
unsigned long lastOpenTime = 0;       
const unsigned long TRAVEL_TIME = 10000; 

void initProximityScan() {
    BLEDevice::init("PawlarDoor");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true); 
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  
}

void scanForCollar() {
    // --- 1. THE AUTOMATION STATE MACHINE ---
    if (doorIsOpen) {
        unsigned long elapsed = millis() - lastOpenTime;
        
        // PHASE: OPENING (First 10 Seconds)
        if (elapsed < TRAVEL_TIME) {
            moveUp(); 
        } 
        // PHASE: WAITING AT TOP (Next 5 Seconds)
        else if (elapsed >= TRAVEL_TIME && elapsed < (TRAVEL_TIME + 5000)) {
            stopMotors(); 
            // Optional: Log once when we hit the pause
            static bool loggedPause = false;
            if(!loggedPause) { Serial.println("⏸️ Door at top. Waiting 5s..."); loggedPause = true; }
        }
        // PHASE: CLOSING (After 15 seconds total)
        else if (elapsed >= (TRAVEL_TIME + 5000) && elapsed < (TRAVEL_TIME * 2 + 5000)) {
            if (isPathClear) {
                moveDown();
            } else {
                // SAFETY OVERRIDE: If blocked, stop and "reset" the timer 
                // to stay in the Waiting Phase until clear.
                stopMotors();
                lastOpenTime = millis() - TRAVEL_TIME; 
                Serial.println("⚠️ OBSTACLE! Holding door open...");
            }
        }
        // PHASE: FINISHED
        else {
            stopMotors();
            doorIsOpen = false;
            isMoving = false; // Tell main.cpp we are totally done
            Serial.println("🔒 Cycle Complete. Door is Closed.");
        }
    }

    // --- 2. THE SCANNER (Triggers the State Machine) ---
    String authList = getAuthorizedCollarList();
    if (authList == "") return; 

    BLEScan* pBLEScan = BLEDevice::getScan();
    // Use a non-blocking scan if possible, or keep this short
    BLEScanResults foundDevices = pBLEScan->start(1, false); 

    for (int i = 0; i < foundDevices.getCount(); i++) {
        BLEAdvertisedDevice device = foundDevices.getDevice(i);
        String foundName = device.getName().c_str(); 
        int rssi = device.getRSSI();

        if (authList.indexOf(foundName) != -1 && foundName.length() > 0) {
            // Only trigger if door is currently fully CLOSED
            if (rssi >= RSSI_THRESHOLD_OPEN && !doorIsOpen) {
                Serial.println("🔓 Proximity Match! Starting Auto-Cycle...");
                doorIsOpen = true;
                isMoving = true;
                lastOpenTime = millis(); // Reset the clock to 0
                moveUp(); 
            }
        }
    }
    pBLEScan->clearResults();
}