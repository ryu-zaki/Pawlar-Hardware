#include "proximity_manager.h"
#include "network_manager.h"
#include "storage_manager.h"
#include "lock_manager.h"
#include "safety_manager.h"
#include <BLEDevice.h>
#include <Arduino.h>

extern bool isPathClear;
extern volatile bool isMoving;
extern bool petHasPassed; // ADDED: To know when the pet has passed through
extern String authorizedCollarsCache;

void moveUp();
void moveDown();
void stopMotors();

// --- CONFIG ---
const int RSSI_THRESHOLD_OPEN = -75; // Slightly more forgiving for better responsiveness
const int RSSI_THRESHOLD_CLOSE = -90; // More forgiving threshold for closing
const unsigned long TRAVEL_TIME = 10000; // 10 seconds
const unsigned long COLLAR_TIMEOUT = 2000; // 2 seconds
const unsigned long WAITING_TIMEOUT = 5000; // 5 seconds for door to wait before closing

// --- STATE MACHINE ---
DoorState currentDoorState = DOOR_IDLE;
unsigned long currentPositionMs = 0; // 0 = closed, TRAVEL_TIME = fully open
unsigned long lastUpdateTime = 0;

unsigned long doorCycleStartTime = 0;
unsigned long waitingStartTime = 0; // Added: To track when the door entered WAITING state
int lastSeenRssi = -100;
unsigned long lastSeenCollarTime = 0;
String lastSeenCollarId = "";

// --- REMOTE OVERRIDE STATE ---
bool pendingManualConfirmation = false;
String manualConfirmationState = "";
bool manualOverrideActive = false; // Flag to disable auto-close

void handleRemoteCommand(String state) {
    if (state == "OPEN") {
        Serial.println("🌐 MQTT CMD: OPENing Door (Manual Override)...");
        currentDoorState = DOOR_OPENING;
        isMoving = true;
        manualOverrideActive = true; // Stay open until further notice
        moveUp();
        pendingManualConfirmation = true;
        manualConfirmationState = "OPEN";
    } else if (state == "CLOSED") {
        Serial.println("🌐 MQTT CMD: CLOSING Door (Manual Override)...");
        currentDoorState = DOOR_CLOSING;
        isMoving = true;
        manualOverrideActive = false; // Reset override when closing
        moveDown();
        pendingManualConfirmation = true;
        manualConfirmationState = "CLOSED";
    }
}

void initProximityScan() {
    String doorName = "DOOR_" + getUniqueDoorID();
    BLEDevice::init(doorName.c_str());
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void scanForCollar() {
    unsigned long now = millis();
    unsigned long dt = (lastUpdateTime == 0) ? 0 : now - lastUpdateTime;
    lastUpdateTime = now;

    String authList = authorizedCollarsCache;
    if (authList == "") {
        return;
    }

    static unsigned long lastDebugPrint = 0;
    if (millis() - lastDebugPrint > 5000) {
        Serial.println("DEBUG: Current Auth List: [" + authList + "]");
        lastDebugPrint = millis();
    }

    // --- Always be scanning to keep RSSI fresh ---
    BLEScan* pBLEScan = BLEDevice::getScan();
    BLEScanResults foundDevices = pBLEScan->start(1, false);

    bool authorizedCollarFound = false;
    for (int i = 0; i < foundDevices.getCount(); i++) {
        BLEAdvertisedDevice device = foundDevices.getDevice(i);
        String foundName = device.getName().c_str();
        String foundAddr = device.getAddress().toString().c_str();
        foundAddr.toUpperCase(); // Ensure consistency
        
        bool isAuthorized = false;
        if (foundName.length() > 0 && authList.indexOf(foundName) != -1) isAuthorized = true;
        if (authList.indexOf(foundAddr) != -1) isAuthorized = true;

        if (isAuthorized) {
            lastSeenRssi = device.getRSSI();
            lastSeenCollarTime = millis();
            lastSeenCollarId = foundName.length() > 0 ? foundName : foundAddr;
            authorizedCollarFound = true;
            break; 
        }
    }
    pBLEScan->clearResults();


    // --- AUTOMATION STATE MACHINE ---
    switch (currentDoorState) {
        case DOOR_IDLE:
            currentPositionMs = 0;
            if (authorizedCollarFound && lastSeenRssi >= RSSI_THRESHOLD_OPEN) {
                Serial.println("🔓 Proximity Match! Starting Auto-Cycle...");
                publishNotification("Pet Activity", "is detected at the door.", "INFO", lastSeenCollarId);
                currentDoorState = DOOR_OPENING;
                petHasPassed = false; // Reset for the new cycle
                isMoving = true;
                moveUp();
            }
            break;

        case DOOR_OPENING:
            currentPositionMs += dt;
            if (currentPositionMs >= TRAVEL_TIME) {
                currentPositionMs = TRAVEL_TIME;
                stopMotors();
                
                if (manualOverrideActive) {
                    Serial.println("🛑 Manual Open Complete. Holding state.");
                    currentDoorState = DOOR_OPEN;
                } else {
                    Serial.println("🛑 Auto Open Complete. Now waiting.");
                    currentDoorState = DOOR_WAITING;
                    waitingStartTime = millis();
                }

                if (pendingManualConfirmation && manualConfirmationState == "OPEN") {
                    publishDoorConfirmation("OPEN", true);
                    pendingManualConfirmation = false;
                }
            } else {
                moveUp();
            }
            break;

        case DOOR_WAITING:
            currentPositionMs = TRAVEL_TIME;
            
            // IF manual override is active, we NEVER auto-close from this state.
            if (manualOverrideActive) {
                currentDoorState = DOOR_OPEN;
                return;
            }

            // Condition 1: Pet has fully passed through the IR sensors
            if (petHasPassed) {
                Serial.println("🐾 Pet has passed. Starting close sequence.");
                currentDoorState = DOOR_CLOSING;
            }
            // Condition 2: Collar is gone (either out of RSSI range or timed out)
            else if (millis() - lastSeenCollarTime > COLLAR_TIMEOUT || lastSeenRssi < RSSI_THRESHOLD_CLOSE) {
                Serial.println("📡 Collar out of range. Starting close sequence.");
                currentDoorState = DOOR_CLOSING;
            }
            // Condition 3: Waiting time elapsed
            else if (millis() - waitingStartTime >= WAITING_TIMEOUT) {
                Serial.println("⏳ Waiting time elapsed. Starting close sequence.");
                currentDoorState = DOOR_CLOSING;
            }
            break;

        case DOOR_OPEN:
            currentPositionMs = TRAVEL_TIME;
            // Stay here until a manual CLOSE command or physical button press changes the state
            break;

        case DOOR_CLOSING:
            if (isPathClear) {
                moveDown();
                if (currentPositionMs > dt) {
                    currentPositionMs -= dt;
                } else {
                    currentPositionMs = 0;
                }
            } else {
                stopMotors();
                Serial.println("⚠️ OBSTACLE! Pausing close.");
                currentDoorState = DOOR_WAITING;
                waitingStartTime = millis(); 
                return;
            }

            // Check if closing is complete
            if (currentPositionMs == 0) {
                Serial.println("🔒 Cycle Complete. Door is Closed.");
                stopMotors();
                isMoving = false;
                currentDoorState = DOOR_IDLE;
                publishNotification("Door Locked", "has been locked.", "INFO");

                if (pendingManualConfirmation && manualConfirmationState == "CLOSED") {
                    publishDoorConfirmation("CLOSED", true);
                    pendingManualConfirmation = false;
                }
            }
            break;
    }
}