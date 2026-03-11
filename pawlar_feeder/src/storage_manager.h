#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>

void initStorage();
void clearStorage();
void saveCredentials(String ssid, String pass);
String getSSID();
String getPass();
void saveAuthorizedCollar(String collarList);
String getAuthorizedCollarList();
String getDeviceId();
String getUniqueFeederID();

#endif
