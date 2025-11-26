/**
 * @file main.cpp
 * @brief Pawlar Collar Master Controller
 */
#include <Arduino.h>
#include "config.h"
#include "storage_manager.h"
#include "gps_manager.h"
#include "network_manager.h"
#include "ble_manager.h"

bool pairingMode = false;
volatile bool btnPressed = false;
void IRAM_ATTR isr() { btnPressed = true; }

void setup() {
    Serial.begin(115200);
    Serial.println("\n🚀 Pawlar System Starting...");
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, HIGH);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), isr, FALLING);
    
    initGPS();
    pairingMode = isPairingRequested();
    initBLE(pairingMode);
    
    Serial.println("🆔 ID: " + getUniqueDeviceID());

    if (!pairingMode) {
        String s = getSSID(); String p = getPass();
        if (s != "") connectToCloud(s, p);
    }
}

void loop() {
    readGPS();
    if (!pairingMode) runCloud();

    static unsigned long lastLog = 0;
    if (millis() - lastLog > GPS_LOG_INTERVAL) {
        if (hasFix()) {
            if (!pairingMode) {
                Serial.printf("📍 Lat: %.6f, Lng: %.6f\n", getLat(), getLng());
                sendLocationData(getLat(), getLng(), getSatellites());
            }
        } else if (!pairingMode) Serial.print(".");
        lastLog = millis();
    }

    if (btnPressed) {
        delay(50);
        if (digitalRead(BUTTON_PIN) == LOW) {
            unsigned long start = millis();
            while (digitalRead(BUTTON_PIN) == LOW) {
                digitalWrite(LED_PIN, !digitalRead(LED_PIN)); delay(100);
            }
            if (millis() - start > LONG_PRESS_TIME) {
                setPairingRequest(!pairingMode);
                delay(500); ESP.restart();
            }
        }
        btnPressed = false;
    }
}