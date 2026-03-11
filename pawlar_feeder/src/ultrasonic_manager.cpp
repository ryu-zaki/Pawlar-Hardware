#include "ultrasonic_manager.h"

UltrasonicManager::UltrasonicManager(int trigPin, int echoPin) 
    : _trigPin(trigPin), _echoPin(echoPin) {}

void UltrasonicManager::begin() {
    pinMode(_trigPin, OUTPUT);
    pinMode(_echoPin, INPUT);
}

float UltrasonicManager::getDistance() {
    digitalWrite(_trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigPin, LOW);

    long duration = pulseIn(_echoPin, HIGH, 30000); // 30ms timeout
    if (duration == 0) return -1.0;

    // Speed of sound is ~0.034 cm/us
    float distance = (duration * 0.034) / 2;
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
