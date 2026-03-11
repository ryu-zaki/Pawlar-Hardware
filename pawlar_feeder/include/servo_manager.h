#ifndef SERVO_MANAGER_H
#define SERVO_MANAGER_H

#include <ESP32Servo.h>

class ServoManager {
public:
    ServoManager(int pin);
    void begin();
    void open();
    void close();
    void dispense();

private:
    int _pin;
    Servo _servo;
};

#endif
