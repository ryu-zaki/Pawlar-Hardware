#include "ble_manager.h"
#include "config.h"
#include "storage_manager.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

void unlockDoor() {
    Serial.println("🔓 [HARDWARE]: Servos Rotating to UNLOCK position...");
}