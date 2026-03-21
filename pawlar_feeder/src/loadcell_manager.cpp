#include "loadcell_manager.h"

LoadCellManager::LoadCellManager(int dt_pin, int sck_pin) 
    : _dt_pin(dt_pin), _sck_pin(sck_pin) {}

void LoadCellManager::begin() {
    scale.begin(_dt_pin, _sck_pin);
    
    Serial.println("⚖️ Initializing Load Cell...");
    
    // Give it a longer moment to stabilize power
    delay(1000); 

    if (scale.wait_ready_timeout(5000)) { // Increased timeout to 5s
        scale.set_scale(_calibration_factor);
        
        // More tares for better stability
        Serial.println("⚖️ Taring...");
        scale.tare(30); 
        Serial.println("✅ Load Cell Ready and Tared.");
    } else {
        Serial.println("❌ HX711 not found. Check DT/SCK wiring!");
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
