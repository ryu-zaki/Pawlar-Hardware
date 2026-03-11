#include "battery_manager.h"
#include "network_manager.h"
#include <Arduino.h>

// Using 100k (R1) and 10k (R2) divider
const float VOLTAGE_MULTIPLIER = 12.36;
const int BATTERY_PIN = 35;

void initBatteryMonitor() {
    pinMode(BATTERY_PIN, INPUT); 
    Serial.println("✅ Battery Monitor Initialized (Resistor Divider on GPIO 35)");
}

float getVoltage() {
    // Read 10 times and average for stability
    long sum = 0;
    for(int i = 0; i < 10; i++) {
        sum += analogRead(BATTERY_PIN);
    }
    float rawAverage = sum / 10.0;
    
    // 🚩 DEBUG: Print raw value to Serial
    Serial.printf("📊 Raw ADC Average: %.2f\n", rawAverage);
    
    // 3.3V / 4095 * 12.36 (Calculated Multiplier)
    float voltage = (rawAverage * 3.3 / 4095.0) * 12.36;
    return voltage;
}
int calculateBatteryPercentage(float voltage) {
    float minV = 9.6;  // 3S Empty
    float maxV = 12.6; // 3S Full
    int percentage = (int)((voltage - minV) / (maxV - minV) * 100);
    percentage = constrain(percentage, 0, 100);
    // Round to nearest 10 for cleaner reporting
    return (percentage / 10) * 10;
}

void reportBatteryHealth() {
    float voltage = getVoltage();
    int batPercent = calculateBatteryPercentage(voltage);
    
    Serial.printf("🔋 [BATTERY]: %.2fV (%d%%)\n", voltage, batPercent);
    
    // We send 0.0 for current since resistors can't measure mA
    publishBatteryHealth(voltage, 0.0, batPercent); 
}