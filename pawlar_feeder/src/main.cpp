#include <Arduino.h>
#include <ESP32Servo.h>

// Updated to "Safe" Pins (GPIO 14 and 27)
const int BUTTON_PIN = 14; 
const int SERVO_PIN  = 27;

Servo myServo;

void setup() {
    Serial.begin(115200);
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    myServo.setPeriodHertz(50); 
    myServo.attach(SERVO_PIN, 500, 2400); 
    
    Serial.println("System Initialized (New Pins). Press button on GPIO 14.");
    myServo.write(0); 
}

void loop() {
    if (digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("Button Pressed! Moving Servo...");
        myServo.write(90); 
        delay(1000);
        myServo.write(0);  
        delay(500);
    }
}
