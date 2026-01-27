#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H
#include <Arduino.h>

void initStorage(); // Add this!
void saveCredentials(String ssid, String pass);
void saveAuthorizedCollar(String collarID);
String getSSID();
String getPass();
String getAuthorizedCollar();

#endif