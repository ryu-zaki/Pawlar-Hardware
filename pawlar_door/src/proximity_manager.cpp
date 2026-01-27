#include "proximity_manager.h"
#include "network_manager.h"
#include "storage_manager.h"
#include "network_manager.h"
#include "lock_manager.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <math.h>
#include <Arduino.h>

const String targetCollarID = "COLLAR_9BD9B4DC"; 
const int RSSI_THRESHOLD = -65; // Adjust this for your desired distance (approx 1-2 meters)

// Function to estimate distance in meters based on RSSI
double calculateDistance(int rssi) {
    int txPower = -59; // Hardcoded reference RSSI at 1 meter
    if (rssi == 0) return -1.0;
    float ratio = rssi * 1.0 / txPower;
    if (ratio < 1.0) return pow(ratio, 10);
    return (0.89976) * pow(ratio, 7.7095) + 0.111;
}

void initProximityScan() {
    BLEDevice::init("PawlarDoor");
}

void scanForCollar() {
    String whiteListedID = getAuthorizedCollar();
    
    // If no collar is registered yet, don't trigger anything
    if (whiteListedID == "") return; 

    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    BLEScanResults foundDevices = pBLEScan->start(1, false);

    for (int i = 0; i < foundDevices.getCount(); i++) {
        BLEAdvertisedDevice device = foundDevices.getDevice(i);
        
        // CHECK: Does this device match our registered collar?
        if (String(device.getName().c_str()) == whiteListedID) {
            int rssi = device.getRSSI();
            double distance = calculateDistance(rssi);

            if (rssi > RSSI_THRESHOLD) {
                logTriggerEvent(rssi, distance);
                unlockDoor(); // From lock_manager
            }
        }
    }
    pBLEScan->clearResults();
}