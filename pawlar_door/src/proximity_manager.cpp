#include "proximity_manager.h"
#include "network_manager.h"
#include "storage_manager.h"
#include "lock_manager.h"
#include "safety_manager.h"
#include <BLEDevice.h>
#include <Arduino.h>

extern bool isPathClear;
extern bool isMoving;
extern bool petHasPassed; // ADDED: To know when the pet has passed through

void moveUp();
void moveDown();
void stopMotors();

// --- CONFIG ---
const int RSSI_THRESHOLD_OPEN = -75;
const int RSSI_THRESHOLD_CLOSE = -90; // More forgiving threshold for closing
const unsigned long TRAVEL_TIME = 10000; // 10 seconds
const unsigned long COLLAR_TIMEOUT = 2000; // 2 seconds
const unsigned long WAITING_TIMEOUT = 5000; // 5 seconds for door to wait before closing

// --- STATE MACHINE ---
enum DoorState { DOOR_IDLE, DOOR_OPENING, DOOR_WAITING, DOOR_CLOSING };
DoorState currentDoorState = DOOR_IDLE;

unsigned long doorCycleStartTime = 0;
unsigned long waitingStartTime = 0; // Added: To track when the door entered WAITING state
int lastSeenRssi = -100;
unsigned long lastSeenCollarTime = 0;

void initProximityScan() {
    BLEDevice::init("pawlar door setup");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void scanForCollar() {
    String authList = getAuthorizedCollarList();
    if (authList == "") return;

    // --- Always be scanning to keep RSSI fresh ---
    BLEScan* pBLEScan = BLEDevice::getScan();
    BLEScanResults foundDevices = pBLEScan->start(1, false);

    bool authorizedCollarFound = false;
    for (int i = 0; i < foundDevices.getCount(); i++) {
        BLEAdvertisedDevice device = foundDevices.getDevice(i);
        String foundName = device.getName().c_str();

        if (authList.indexOf(foundName) != -1 && foundName.length() > 0) {
            lastSeenRssi = device.getRSSI();
            lastSeenCollarTime = millis();
            authorizedCollarFound = true;
            break; 
        }
    }
    pBLEScan->clearResults();


    // --- AUTOMATION STATE MACHINE ---
    switch (currentDoorState) {
        case DOOR_IDLE:
            if (authorizedCollarFound && lastSeenRssi >= RSSI_THRESHOLD_OPEN) {
                Serial.println("🔓 Proximity Match! Starting Auto-Cycle...");
                currentDoorState = DOOR_OPENING;
                doorCycleStartTime = millis();
                petHasPassed = false; // Reset for the new cycle
                isMoving = true;
                moveUp();
            }
            break;

        case DOOR_OPENING:
            if (millis() - doorCycleStartTime >= TRAVEL_TIME) {
                Serial.println("🛑 Door has reached the top. Now waiting.");
                stopMotors();
                currentDoorState = DOOR_WAITING;
                waitingStartTime = millis(); // Set waiting start time
            }
            break;

        case DOOR_WAITING:
            // Condition 1: Pet has fully passed through the IR sensors
            if (petHasPassed) {
                Serial.println("🐾 Pet has passed. Starting close sequence.");
                currentDoorState = DOOR_CLOSING;
                doorCycleStartTime = millis(); // Reset timer for closing
            }
            // Condition 2: Collar is gone (either out of RSSI range or timed out)
            else if (millis() - lastSeenCollarTime > COLLAR_TIMEOUT || lastSeenRssi < RSSI_THRESHOLD_CLOSE) {
                Serial.println("📡 Collar out of range. Starting close sequence.");
                currentDoorState = DOOR_CLOSING;
                doorCycleStartTime = millis(); // Reset timer for closing
            }
            // Condition 3: Waiting time elapsed
            else if (millis() - waitingStartTime >= WAITING_TIMEOUT) {
                Serial.println("⏳ Waiting time elapsed. Starting close sequence.");
                currentDoorState = DOOR_CLOSING;
                doorCycleStartTime = millis(); // Reset timer for closing
            }
            break;

        case DOOR_CLOSING:
            if (isPathClear) {
                moveDown();
            } else {
                stopMotors();
                Serial.println("⚠️ OBSTACLE! Pausing close.");
                // To prevent it from immediately trying to close again, we can go back to waiting
                currentDoorState = DOOR_WAITING;
                waitingStartTime = millis(); // Reset waiting start time when returning to WAITING due to obstacle
                return;
            }

            // Check if closing is complete
            if (millis() - doorCycleStartTime >= TRAVEL_TIME) {
                Serial.println("🔒 Cycle Complete. Door is Closed.");
                stopMotors();
                isMoving = false;
                currentDoorState = DOOR_IDLE;
            }
            break;
    }
}