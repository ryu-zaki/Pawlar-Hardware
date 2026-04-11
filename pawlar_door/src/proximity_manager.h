#ifndef PROXIMITY_MANAGER_H
#define PROXIMITY_MANAGER_H
#include <Arduino.h>

enum DoorState { DOOR_IDLE, DOOR_OPENING, DOOR_WAITING, DOOR_CLOSING, DOOR_OPEN };

extern String lastSeenCollarId;

void initProximityScan();
void scanForCollar();
void updateDoorAutomation();
void handleRemoteCommand(String state); // Handle manual state changes from MQTT
#endif