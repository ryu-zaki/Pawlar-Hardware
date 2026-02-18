#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>

extern PubSubClient client; 

bool connectToWiFi(String ssid, String pass);
void initNetwork();
void logTriggerEvent(int rssi, double distance);
void publishDoorActivity(String event, double distance);
void publishBatteryHealth(float voltage, float current, int percentage);
#endif