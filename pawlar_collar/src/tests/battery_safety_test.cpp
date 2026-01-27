#include <Arduino.h>
#include <Wire.h>
#include <DFRobot_MAX17043.h>

// I2C Pins for SuperMini
#define PIN_SDA 4
#define PIN_SCL 5
#define LED_PIN 8  // Onboard LED for physical warning

DFRobot_MAX17043 gauge;

void setup() {
    // 1. Start Serial
    Serial.begin(115200);

    // 2. ✋ THE FIX: Wait for the USB Connection
    // This loop pauses the code until you actually open the monitor
    unsigned long start = millis();
    while (!Serial && millis() - start < 5000) {
        delay(10); 
    }

    // 3. Init Hardware
    Wire.begin(PIN_SDA, PIN_SCL);
    pinMode(8, OUTPUT); // Onboard LED

    Serial.println("\n\n*****************************************");
    Serial.println("🚀 PAWLAR BATTERY TEST IS ALIVE!");
    Serial.println("*****************************************");

    if (gauge.begin() != 0) {
        Serial.println("❌ ERROR: MAX17043 NOT FOUND");
    } else {
        Serial.println("✅ SENSOR CONNECTED");
    }
}

void loop() {
    // 4. Read Data
    float voltage = gauge.readVoltage();
    int percentage = (int)gauge.readPercentage();

    // 5. Print to Serial Monitor
    Serial.print("🔋 Battery: ");
    Serial.print(percentage);
    Serial.print("% | ");
    Serial.print(voltage, 0);
    Serial.print("mV (");
    Serial.print(voltage / 1000.0, 2);
    Serial.println("V)");

    // 6. SAFETY ALERT: Manually unplug if voltage is too low!
    if (voltage < 3300) {
        Serial.println("⚠️ !!! CRITICAL LOW BATTERY !!!");
        Serial.println("⚠️ UNPLUG THE BATTERY IMMEDIATELY TO PREVENT DAMAGE.");
        
        // Blink onboard LED rapidly for a physical alarm
        digitalWrite(LED_PIN, LOW); delay(100);
        digitalWrite(LED_PIN, HIGH); delay(100);
    } else {
        Serial.println("✅ Status: SAFE");
        digitalWrite(LED_PIN, HIGH); // Keep LED off if safe
    }

    Serial.println("-------------------------------------------------");
    delay(1500); // Check every 1.5 seconds
}