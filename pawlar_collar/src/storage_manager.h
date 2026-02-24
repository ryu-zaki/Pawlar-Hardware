#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H
#include <Arduino.h>
void initStorage();
void saveWiFiCreds(String ssid, String pass);
String getSSID();
String getPass();
bool isPairingRequested();
void setPairingRequest(bool enable);
String getUniqueDeviceID();
String getMACAddress();
#endif