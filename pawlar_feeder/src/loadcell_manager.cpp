#include "loadcell_manager.h"

LoadCellManager::LoadCellManager(int dt_pin, int sck_pin) 
    : _dt_pin(dt_pin), _sck_pin(sck_pin) {}

void LoadCellManager::begin() {
    scale.begin(_dt_pin, _sck_pin);
    
    Serial.printf("⚖️ Initializing HX711 (DT:%d, SCK:%d)...\n", _dt_pin, _sck_pin);
    
    // Safety check for pins
    if (_dt_pin == 0 || _sck_pin == 0) {
        Serial.println("❌ ERROR: Pins not defined! Check config.h");
        return;
    }

    if (scale.wait_ready_timeout(5000)) { 
        scale.set_scale(_calibration_factor);
        
        Serial.println("⚖️ Stabilization...");
        delay(500); // 0.5s settle time

        Serial.println("⚖️ Taring bowl (30 samples)...");
        scale.tare(30); 
        Serial.printf("✅ Ready! Factor: %.2f\n", _calibration_factor);
    } else {
        Serial.println("❌ HX711 not found. Pins correct? Is VCC 5V?");
    }
}

float LoadCellManager::getWeight(int readings) {
    if (scale.is_ready()) {
        return scale.get_units(readings);
    } else {
        return -1.0f;
    }
}

void LoadCellManager::tare() {
    if (scale.is_ready()) {
        scale.tare();
        Serial.println("⚖️ Scale Tared.");
    }
}

void LoadCellManager::setScale(float scale_factor) {
    _calibration_factor = scale_factor;
    scale.set_scale(_calibration_factor);
}

bool LoadCellManager::isReady() {
    return scale.is_ready();
}
