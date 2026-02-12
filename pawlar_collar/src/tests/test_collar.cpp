#include <Arduino.h>

// Updated to match your physical wiring
#define GPS_RX 6  // Connect to GPS TX
#define GPS_TX 7  // Connect to GPS RX

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("🛰️ GPS Satellite Lock Test Initializing...");

    // This tells the ESP32 to 'listen' on Pin 6
    Serial1.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX); 
    
    Serial.println("👀 Listening for NMEA Data. GPS TX -> ESP32 Pin 6");
}

void loop() {
    while (Serial1.available()) {
        char c = Serial1.read();
        Serial.print(c); 
    }

    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 5000) {
        if (!Serial1.available()) {
            Serial.println("\n⚠️ Still no data. Trying to flip logic in code...");
            // Temporary software flip to test if labels are reversed
            // Serial1.begin(9600, SERIAL_8N1, 7, 6); 
        }
        lastCheck = millis();
    }
}