#ifndef ULTRASONIC_MANAGER_H
#define ULTRASONIC_MANAGER_H

#include <Arduino.h>

enum FeederState {
    STATE_HIGH,
    STATE_MID,
    STATE_LOW,
    STATE_EMPTY,
    STATE_UNKNOWN
};

class UltrasonicManager {
public:
    UltrasonicManager(int trigPin, int echoPin);
    void begin();
    float getDistance();
    FeederState getState();
    String stateToString(FeederState state);

private:
    int _trigPin;
    int _echoPin;
    const float MAX_DISTANCE = 12.0;
};

#endif
