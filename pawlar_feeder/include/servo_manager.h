#ifndef SERVO_MANAGER_H
#define SERVO_MANAGER_H

#include <ESP32Servo.h>
#include "loadcell_manager.h"

class ServoManager {
public:
    ServoManager(int dispenserPin, int bowlPin);
    void begin();
    
    // Dispenser Methods
    void openDispenser();
    void closeDispenser();
    
    // Bowl Blockage Methods ("Toilet Seat" motion)
    void openBowl();
    void closeBowl();

    bool dispenseWeight(float targetGrams, LoadCellManager& loadCell, bool closeAtEnd = true);
    bool dispenseWithBlockage(float targetGrams, LoadCellManager& loadCell);

private:
    int _dispenserPin;
    int _bowlPin;
    Servo _dispenserServo;
    Servo _bowlServo;
};

#endif
