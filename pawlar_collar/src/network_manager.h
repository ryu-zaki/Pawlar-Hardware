#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H
#include <Arduino.h>
void connectToCloud(String ssid, String pass);
void runCloud();
void sendLocationData(float lat, float lng, int sats);
#endif