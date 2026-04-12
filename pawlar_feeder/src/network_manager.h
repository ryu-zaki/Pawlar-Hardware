#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>

extern PubSubClient client;

void initNetwork();
bool connectToWiFi(String ssid, String pass);
void publishFeederActivity(String event, float data);
void publishFeederConfirmation(String command, float amount, bool confirmed);
void publishNotification(String title, String description, String type, String trigger_id = "");

#endif
