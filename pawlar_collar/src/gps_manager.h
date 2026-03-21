#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H
#include <Arduino.h>

extern HardwareSerial gpsSerial;

void initGPS();
void readGPS();
bool hasFix();
float getLat();
float getLng();
int getSatellites();
#endif