#include "gps_manager.h"
#include "config.h"
#include <TinyGPSPlus.h>

TinyGPSPlus gps;
// ESP32-C3 only has Serial 0 (USB) and Serial 1. HardwareSerial(2) will crash.
HardwareSerial gpsSerial(1); 

void initGPS() { 
    // RX=Pin 3, TX=Pin 2 as per your config.h
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN); 
    Serial.println("🛰️ GPS Serial 1 started on Pins 3(RX) and 2(TX)");
}

void readGPS() { 
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read()); 
    }
}

bool hasFix() { 
    return gps.location.isValid() && gps.location.age() < 2000; 
}

float getLat() { return gps.location.lat(); }
float getLng() { return gps.location.lng(); }
int getSatellites() { return gps.satellites.value(); }