#ifndef CELLULAR_MANAGER_H
#define CELLULAR_MANAGER_H

#include <Arduino.h>
#include <SoftwareSerial.h>

extern SoftwareSerial cellSerial;

void initCellular();
bool sendCellularSMS(String phoneNumber, String message);
bool sendCellularGPS(float lat, float lng, int bat);
bool isModemAlive();
bool sendCellularMQTT(float lat, float lng, int bat);
bool sendWifiStatusCellular(bool isConnected);

#endif