#include <Arduino.h>
#include "config.h"

// This test bypasses the manager and dumps raw NMEA data to Serial Monitor.
// Use this to verify hardware connections and satellite visibility.

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n--- 🛰️ PAWLAR GPS RAW DATA TEST ---");
    Serial.printf("Connecting to GPS at %d baud...\n", GPS_BAUD);
    Serial.printf("RX (ESP32): Pin %d | TX (ESP32): Pin %d\n", GPS_RX_PIN, GPS_TX_PIN);

    // Initialize Serial1 for GPS
    Serial1.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN); 
    
    Serial.println("👀 Listening for NMEA Data. If you see $GPRMC/$GPGGA, the wiring is correct!");
}

void loop() {
    // Pipe data from GPS to USB Serial
    while (Serial1.available()) {
        char c = Serial1.read();
        Serial.print(c); 
    }

    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 10000) {
        Serial.println("\n[System Check] Still listening... (Ensure you are near a window for a fix)");
        lastCheck = millis();
    }
}