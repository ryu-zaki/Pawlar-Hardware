#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>

extern PubSubClient client;

void initNetwork();
bool connectToWiFi(String ssid, String pass);
void publishFeederActivity(String event, float data);

#endif
