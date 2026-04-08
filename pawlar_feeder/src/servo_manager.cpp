#include "servo_manager.h"
#include <Arduino.h>

ServoManager::ServoManager(int dispenserPin, int bowlPin) : _dispenserPin(dispenserPin), _bowlPin(bowlPin) {}

void ServoManager::begin() {
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    _dispenserServo.setPeriodHertz(50);
    _dispenserServo.attach(_dispenserPin, 500, 2400); 
    _dispenserServo.write(180); // Closed (Standard)

    _bowlServo.setPeriodHertz(50);
    _bowlServo.attach(_bowlPin, 500, 2400); 
    _bowlServo.write(0); // Closed (Standard)
}

void ServoManager::openDispenser() {
    Serial.println("📂 Opening dispenser (Standard)...");
    _dispenserServo.write(0); 
}

void ServoManager::closeDispenser() {
    Serial.println("📁 Closing dispenser (Standard)...");
    _dispenserServo.write(180); 
}

void ServoManager::openBowl() {
    Serial.println("🔓 Lifting bowl blockage (Standard Servo UP)...");
    _bowlServo.write(90);  
}

void ServoManager::closeBowl() {
    Serial.println("🔒 Lowering bowl blockage (Standard Servo DOWN)...");
    _bowlServo.write(0);    
}



bool ServoManager::dispenseWeight(float targetGrams, LoadCellManager& loadCell, bool closeAtEnd) {
    if (!loadCell.isReady()) {
        Serial.println("❌ Load Cell not ready. Aborting dispense.");
        return false;
    }

    float initialWeight = loadCell.getWeight(10);
    
    // Safety check for extreme invalid values
    if (abs(initialWeight) > 5000) {
        Serial.printf("❌ INVALID WEIGHT DETECTED: %.2fg. Check scale wiring!\n", initialWeight);
        return false;
    }

    // Drift correction: if it's slightly negative, we treat it as 0 for relative calculation
    if (initialWeight < -2.0) { // More precise drift check
        Serial.printf("⚠️ SCALE DRIFT DETECTED: %.2fg. Taring and continuing...\n", initialWeight);
        loadCell.tare();
        initialWeight = 0;
    }

    // 🚩 REFILL LOGIC: targetGrams is the desired final bowl weight
    if (initialWeight >= targetGrams) {
        Serial.printf("✅ BOWL FULL: Current weight (%.2fg) >= Target (%.2fg). No dispense needed.\n", initialWeight, targetGrams);
        if (closeAtEnd) closeDispenser();
        return true; 
    }

    float toDispense = targetGrams - initialWeight;
    Serial.printf("⚖️ Weight Check: %.2fg | Refilling to: %.2fg (Adding: %.2fg)\n", initialWeight, targetGrams, toDispense);
    
    float targetWeight = targetGrams; // Dispense until we reach this total weight
    float currentWeight = initialWeight;
    unsigned long startTime = millis();
    const unsigned long timeout = 60000; // 60s safety timeout

    openDispenser();
    
    while (currentWeight < targetWeight) {
        if (millis() - startTime > timeout) {
            Serial.println("⚠️ Safety: Dispense timeout reached!");
            closeDispenser();
            return false;
        }
        
        delay(500); // Wait 0.5s for scale to settle
        currentWeight = loadCell.getWeight(10); 
        Serial.printf("📈 Refilling... Current Weight: %.2fg | Goal: %.2fg\n", currentWeight, targetWeight);
    }

    Serial.println("✅ Target weight reached.");
    
    if (closeAtEnd) {
        closeDispenser();
    }
    return true;
}

bool ServoManager::dispenseWithBlockage(float targetGrams, LoadCellManager& loadCell) {
    Serial.println("🚀 Managed Dispense Cycle Started...");
    
    // 1. Blockage stays/is closed
    closeBowl();
    delay(500);

    // 2. Start dispensing (passing false so it stays open at end of weight check)
    if (dispenseWeight(targetGrams, loadCell, false)) {
        // 3. User Requested: Open blockage, then close dispenser
        Serial.println("✅ Releasing food! Opening blockage THEN stopping dispenser.");
        openBowl();
        delay(300); // Give it a moment to fall
        closeDispenser();
        return true;
    } else {
        Serial.println("❌ Dispense FAILED. Closing everything for safety.");
        closeDispenser();
        // Bowl stays closed
        return false;
    }
}

