#ifndef SERVO_MANAGER_H
#define SERVO_MANAGER_H

#include <ESP32Servo.h>
#include "loadcell_manager.h"

class ServoManager {
public:
    ServoManager(int pin);
    void begin();
    void open();
    void close();
    void dispense(); // Keep for backward compatibility or simple dispense
    void dispenseWeight(float targetGrams, LoadCellManager& loadCell);

private:
    int _pin;
    Servo _servo;
};

#endif
