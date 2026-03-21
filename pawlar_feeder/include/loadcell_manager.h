#ifndef LOADCELL_MANAGER_H
#define LOADCELL_MANAGER_H

#include <Arduino.h>
#include "HX711.h"

class LoadCellManager {
public:
    LoadCellManager(int dt_pin, int sck_pin);
    void begin();
    float getWeight(int readings = 10);
    void tare();
    void setScale(float scale);
    bool isReady();

private:
    HX711 scale;
    int _dt_pin;
    int _sck_pin;
    float _calibration_factor = 419.8; // Default, needs user calibration
};

#endif
