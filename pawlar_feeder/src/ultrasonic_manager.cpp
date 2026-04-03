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
    
    // Total 12cm, divided by 4 = 3cm intervals
    if (distance <= 3.0) {
        return STATE_HIGH;
    } else if (distance <= 6.0) {
        return STATE_MID;
    } else if (distance <= 9.0) {
        return STATE_LOW;
    } else {
        return STATE_EMPTY;
    }
}

String UltrasonicManager::stateToString(FeederState state) {
    switch (state) {
        case STATE_HIGH:    return "HIGH";
        case STATE_MID:     return "MID";
        case STATE_LOW:     return "LOW";
        case STATE_EMPTY:   return "EMPTY";
        default:            return "UNKNOWN";
    }
}
