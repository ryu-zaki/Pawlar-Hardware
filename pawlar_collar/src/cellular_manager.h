#ifndef CELLULAR_MANAGER_H
#define CELLULAR_MANAGER_H

#include <Arduino.h>
#include <HardwareSerial.h>

extern HardwareSerial cellSerial;

void initCellular();
bool sendCellularSMS(String phoneNumber, String message);
bool sendCellularGPS(float lat, float lng, int bat);
bool isModemAlive();
bool sendCellularMQTT(float lat, float lng, int bat);
bool sendWifiStatusCellular(bool isConnected);

#endif