#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H
#include <Arduino.h>

void initStorage();
void saveCredentials(String ssid, String pass);
String getSSID();
String getPass();

void saveDeviceInfo(String deviceId, String userId);
String getDeviceId();
String getUserId();
String getUniqueDoorID();

void saveAuthorizedCollar(String collarList);
String getAuthorizedCollarList();

#endif