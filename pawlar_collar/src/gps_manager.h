#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H
#include <Arduino.h>
void initGPS();
void readGPS();
bool hasFix();
float getLat();
float getLng();
int getSatellites();
#endif