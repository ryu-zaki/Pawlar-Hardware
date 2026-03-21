#include "gps_manager.h"
#include "config.h"
#include <TinyGPSPlus.h>

TinyGPSPlus gps;
// Use Hardware Serial 1 - matching the successful gps_test.cpp
HardwareSerial gpsSerial(1); 

void initGPS() { 
    // Initialize Hardware Serial on the confirmed working pins
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN); 
    Serial.printf("🛰️ GPS HardwareSerial started on Pins %d(RX) and %d(TX)\n", GPS_RX_PIN, GPS_TX_PIN);
}

void readGPS() { 
    static unsigned long lastDataTime = 0;

    while (gpsSerial.available() > 0) {
        char c = gpsSerial.read();
        gps.encode(c); 
        lastDataTime = millis();
    }

    // Diagnostic: If no data at all for 10 seconds, print a warning
    static unsigned long lastWarning = 0;
    if (millis() - lastDataTime > 10000 && millis() - lastWarning > 10000) {
        Serial.println("⚠️ WARNING: No raw data from GPS module. Check TX/RX wiring!");
        lastWarning = millis();
    }
}

bool hasFix() { 
    return gps.location.isValid() && gps.location.age() < 2000; 
}

float getLat() { return gps.location.lat(); }
float getLng() { return gps.location.lng(); }
int getSatellites() { return gps.satellites.value(); }
