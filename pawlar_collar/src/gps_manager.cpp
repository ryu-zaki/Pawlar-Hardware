#include "gps_manager.h"
#include "config.h"
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

TinyGPSPlus gps;
// Use SoftwareSerial for Pins 3 and 2 to avoid UART0 conflict
HardwareSerial gpsSerial(0);

void initGPS() { 
    gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.printf("🛰️ GPS Initialized on Pins %d(RX), %d(TX) using Hardware UART0.\n", GPS_RX_PIN, GPS_TX_PIN);
}

void readGPS() { 
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read()); 
    }
}

// Add this to check if the library is actually receiving data
bool isGpsCommuncating() {
    return gps.charsProcessed() > 0;
}

bool hasFix() { 
    return gps.location.isValid() && gps.location.age() < 2000; 
}

float getLat() { return gps.location.lat(); }
float getLng() { return gps.location.lng(); }
int getSatellites() { return gps.satellites.value(); }
