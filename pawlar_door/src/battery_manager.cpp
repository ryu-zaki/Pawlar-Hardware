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
    
    // 🚩 QUANTIZATION LOGIC (0-25->25, 26-50->50, 51-75->75, 76-100->100)
    if (percentage <= 25) return 25;
    if (percentage <= 50) return 50;
    if (percentage <= 75) return 75;
    return 100;
}

bool doorLowBatteryNotified = false;
int lastReportedPercent = -1; // Track last sent value to avoid spam

void reportBatteryHealth() {
    float voltage = getVoltage();
    int batPercent = calculateBatteryPercentage(voltage);
    
    Serial.printf("🔋 [BATTERY]: %.2fV (Internal: %d%%, Display: %d%%)\n", voltage, (int)((voltage-9.6)/3.0*100), batPercent);
    
    // 🚩 Only publish to App if the quantized percentage has changed
    if (batPercent != lastReportedPercent) {
        publishBatteryHealth(voltage, 0.0, batPercent); 
        lastReportedPercent = batPercent;
        Serial.printf("📤 Published Quantized Battery: %d%%\n", batPercent);
    }

    if (batPercent <= 25 && !doorLowBatteryNotified) {
        publishNotification("Battery Low", "battery is low. Please check the power source.", "WARNING", "");
        doorLowBatteryNotified = true;
    } else if (batPercent > 25) {
        doorLowBatteryNotified = false;
    }
}

bool isBatteryLow() {
    static bool cachedStatus = false;
    static unsigned long lastCheck = 0;
    
    // Only check every 5 seconds to save CPU
    if (millis() - lastCheck > 5000 || lastCheck == 0) {
        lastCheck = millis();
        float voltage = getVoltage();
        int batPercent = calculateBatteryPercentage(voltage);
        cachedStatus = (batPercent <= 25);
    }
    return cachedStatus;
}