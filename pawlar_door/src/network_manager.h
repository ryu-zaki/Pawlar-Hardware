#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H
#include <Arduino.h>

void initNetwork();
bool connectToWiFi(String ssid, String pass);
void logTriggerEvent(int rssi, double distance); // Must be here!
#endif