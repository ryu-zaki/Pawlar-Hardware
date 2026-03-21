#include "servo_manager.h"
#include <Arduino.h>

ServoManager::ServoManager(int pin) : _pin(pin) {}

void ServoManager::begin() {
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    _servo.setPeriodHertz(50);
    _servo.attach(_pin, 500, 2400);
    _servo.write(180); // Closed position
}

void ServoManager::open() {
    _servo.write(40); // Open position
}

void ServoManager::close() {
    _servo.write(180); // Return to closed position
}

void ServoManager::dispense() {
    Serial.println("Dispensing (1s pulse)...");
    open();
    delay(1000);
    close();
    delay(500);
}

void ServoManager::dispenseWeight(float targetGrams, LoadCellManager& loadCell) {
    if (!loadCell.isReady()) {
        Serial.println("❌ Load Cell not ready. Aborting dispense.");
        return;
    }

    float initialWeight = loadCell.getWeight(10);
    if (initialWeight < 0) {
        Serial.println("❌ Error reading weight. Aborting.");
        return;
    }

    Serial.printf("⚖️ Current Weight: %.2fg | Target to add: %.2fg\n", initialWeight, targetGrams);
    
    float targetWeight = initialWeight + targetGrams;
    float currentWeight = initialWeight;
    int pulses = 0;
    const int maxPulses = 50; // Safety limit to avoid infinite dispensing if jammed

    while (currentWeight < targetWeight && pulses < maxPulses) {
        Serial.printf("📈 Pulse %d: Current Weight: %.2fg | Goal: %.2fg\n", pulses + 1, currentWeight, targetWeight);
        
        open();
        delay(800); // 800ms pulse
        close();
        delay(2000); // Wait 2s for food to land and scale to stabilize
        
        currentWeight = loadCell.getWeight(10);
        pulses++;
        
        if (currentWeight < 0) {
            Serial.println("❌ Scale read error during dispense.");
            break;
        }
    }

    if (pulses >= maxPulses) {
        Serial.println("⚠️ Safety: Max pulses reached. Check for jams or empty container.");
    } else {
        Serial.printf("✅ Success! Final weight: %.2fg (Dispensed %.2fg)\n", currentWeight, currentWeight - initialWeight);
    }
}
