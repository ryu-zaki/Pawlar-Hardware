#include "ultrasonic_manager.h"

UltrasonicManager::UltrasonicManager(int trigPin, int echoPin) 
    : _trigPin(trigPin), _echoPin(echoPin) {}

void UltrasonicManager::begin() {
    pinMode(_trigPin, OUTPUT);
    pinMode(_echoPin, INPUT);
}

float UltrasonicManager::getDistance() {
    long totalDuration = 0;
    int validReadings = 0;

    for (int i = 0; i < 5; i++) {
        digitalWrite(_trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(_trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(_trigPin, LOW);

        long duration = pulseIn(_echoPin, HIGH, 30000); 
        if (duration > 0 && duration < 25000) { // Filter out extreme values
            totalDuration += duration;
            validReadings++;
        }
        delay(20); // Short gap between pulses
    }

    if (validReadings == 0) return -1.0;

    float avgDuration = (float)totalDuration / validReadings;
    float distance = (avgDuration * 0.034) / 2;
    return distance;
}

FeederState UltrasonicManager::getState() {
    float distance = getDistance();
    
    if (distance < 0) return STATE_UNKNOWN;
    
    // Total 12cm, divided by 3 = 4cm intervals
    if (distance <= 4.0) {
        return STATE_FULL;
    } else if (distance <= 8.0) {
        return STATE_WARNING;
    } else {
        return STATE_LOW;
    }
}

String UltrasonicManager::stateToString(FeederState state) {
    switch (state) {
        case STATE_FULL:    return "FULL";
        case STATE_WARNING: return "WARNING";
        case STATE_LOW:     return "LOW";
        default:            return "UNKNOWN";
    }
}
