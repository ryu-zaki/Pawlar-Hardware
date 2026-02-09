#include "safety_manager.h"
#include "network_manager.h"
#include "config.h"

enum PetPath { IDLE, START_IN, START_OUT };
PetPath currentPath = IDLE;
unsigned long pathTimer = 0;
bool bothSensorsBlocked = false; // 🛡️ New: Confirms pet is physically "in the middle"
bool isPathClear = true;

const int VERIFICATION_DELAY = 150;

void initSafetySensors() {
    pinMode(IR_INSIDE_PIN, INPUT_PULLUP);
    pinMode(IR_OUTSIDE_PIN, INPUT_PULLUP);
    Serial.println("🛡️ IR Safety Sensors Verified & Active");
}

bool getStableRead(int pin) {
    if (digitalRead(pin) == LOW) {
        delay(VERIFICATION_DELAY);
        return (digitalRead(pin) == LOW);
    }
    return false;
}

void checkIRActivity() {
    bool insideBlocked = getStableRead(IR_INSIDE_PIN);
    bool outsideBlocked = getStableRead(IR_OUTSIDE_PIN);

    isPathClear = !(insideBlocked || outsideBlocked);

    if (currentPath != IDLE && (millis() - pathTimer > 5000)) {
        currentPath = IDLE;
        bothSensorsBlocked = false;
        Serial.println("🔄 IR Sequence Reset (Idle)");
    }

    switch (currentPath) {
        case IDLE:
            if (outsideBlocked && !insideBlocked) {
                currentPath = START_IN;
                pathTimer = millis();
                bothSensorsBlocked = false;
                Serial.println("🐾 Pet detected: ENTERING...");
            } 
            else if (insideBlocked && !outsideBlocked) {
                currentPath = START_OUT;
                pathTimer = millis();
                bothSensorsBlocked = false;
                Serial.println("🐾 Pet detected: EXITING...");
            }
            break;

        case START_IN:
            // 1. Confirm pet is currently spanning BOTH sensors
            if (outsideBlocked && insideBlocked) {
                bothSensorsBlocked = true;
                pathTimer = millis();
            }
            // 2. Only trigger success if they were spanning both and now both are clear
            if (!insideBlocked && !outsideBlocked) {
                if (bothSensorsBlocked) {
                    Serial.println("✅ SUCCESS: PET FULLY ENTERED");
                    publishDoorActivity("PET_GOING_IN", 0.0);
                }
                currentPath = IDLE;
                bothSensorsBlocked = false;
            }
            break;

        case START_OUT:
            // 1. Confirm pet is currently spanning BOTH sensors
            if (insideBlocked && outsideBlocked) {
                bothSensorsBlocked = true;
                pathTimer = millis();
            }
            // 2. Trigger success only after full transition
            if (!insideBlocked && !outsideBlocked) {
                if (bothSensorsBlocked) {
                    Serial.println("✅ SUCCESS: PET FULLY EXITED");
                    publishDoorActivity("PET_GOING_OUT", 0.0);
                }
                currentPath = IDLE;
                bothSensorsBlocked = false;
            }
            break;
    }
}