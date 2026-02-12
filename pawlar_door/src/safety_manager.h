#ifndef SAFETY_MANAGER_H
#define SAFETY_MANAGER_H

#include <Arduino.h>

extern bool petHasPassed; // ADDED: Global flag for pet passage event

void initSafetySensors();
void checkIRActivity();
void saveDeviceInfo(String deviceId, String userId);
String getDeviceId();
String getUserId();

#endif