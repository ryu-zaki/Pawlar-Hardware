#ifndef PROXIMITY_MANAGER_H
#define PROXIMITY_MANAGER_H
#include <Arduino.h>

extern unsigned long lastDispenseTime;

void initProximityScan();
void scanForCollar();
#endif
