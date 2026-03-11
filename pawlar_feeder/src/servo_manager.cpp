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
    _servo.write(180); // Set to 180 as the new closed/home position
}

void ServoManager::open() {
    _servo.write(40); // Move to 90 to open
}

void ServoManager::close() {
    _servo.write(220); // Return to 180 to close
}

void ServoManager::dispense() {
    Serial.println("Dispensing...");
    open();
    delay(1000);
    close();
    delay(500);
}
