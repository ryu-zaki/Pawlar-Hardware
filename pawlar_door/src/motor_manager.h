#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H
#include <Arduino.h>

void initNetwork();
void logTriggerEvent(int rssi, double distance);
#endif