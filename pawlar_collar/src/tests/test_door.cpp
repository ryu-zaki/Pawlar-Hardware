/**
 * @file test_door.cpp
 * @brief Logic for the Door (BLE Central) with PROXIMITY CHECK
 * * LEDs:
 * - GREEN  (Pin 0): Scanning / Idle
 * - BLUE   (Pin 1): Collar 1 (Close Range)
 * - YELLOW (Pin 2): Collar 2 (Close Range)
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>

// --- PINS ---
#define LED_SCAN_PIN 0    // GREEN
#define LED_C1_PIN   1    // BLUE
#define LED_C2_PIN   2    // YELLOW

// --- 📏 DISTANCE CALIBRATION (CRITICAL!) ---
// RSSI = Signal Strength. Closer = Higher number (less negative).
// -40 is touching. -90 is far away.
// Adjust this number if it triggers too far or too close.
#define RSSI_THRESHOLD  -60   // Approx 10 inches
#define RSSI_BUFFER      5    // Buffer to prevent flickering (Disconnects at -65)

// --- UUIDs ---
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

// Flags & Objects
bool connectedC1 = false;
bool connectedC2 = false;
BLEClient* pClientC1 = nullptr;
BLEClient* pClientC2 = nullptr;
BLEAddress *pAddressC1 = nullptr;
BLEAddress *pAddressC2 = nullptr;
BLEScan* pBLEScan;

// --- HELPER: Connect ---
bool connectToCollar(BLEAddress pAddress, BLEClient* &pClient) {
    if (pClient == nullptr) pClient = BLEDevice::createClient();
    Serial.print("🔗 Connecting to "); Serial.println(pAddress.toString().c_str());
    if (pClient->connect(pAddress)) {
        Serial.println("✅ Connected!");
        return true;
    }
    return false;
}

// --- CALLBACK: Discovery ---
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String name = advertisedDevice.getName().c_str();
        int rssi = advertisedDevice.getRSSI(); // Get Signal Strength

        // Only care if it's our collars
        if (name == "PAWLAR_C1" || name == "PAWLAR_C2") {
            Serial.print("👀 Saw "); Serial.print(name); 
            Serial.print(" at RSSI: "); Serial.println(rssi);

            // 📏 FILTER: Only connect if close enough!
            if (rssi > RSSI_THRESHOLD) { 
                if (name == "PAWLAR_C1" && !connectedC1) {
                    Serial.println("✅ Collar 1 is CLOSE! Connecting...");
                    pAddressC1 = new BLEAddress(advertisedDevice.getAddress());
                    advertisedDevice.getScan()->stop(); 
                }
                else if (name == "PAWLAR_C2" && !connectedC2) {
                    Serial.println("✅ Collar 2 is CLOSE! Connecting...");
                    pAddressC2 = new BLEAddress(advertisedDevice.getAddress());
                    advertisedDevice.getScan()->stop();
                }
            } else {
                // Serial.println("❌ Too far to connect.");
            }
        }
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("🚪 Door System Starting (Proximity Mode)...");

    pinMode(LED_SCAN_PIN, OUTPUT);
    pinMode(LED_C1_PIN, OUTPUT);
    pinMode(LED_C2_PIN, OUTPUT);

    // Lamp Test
    digitalWrite(LED_SCAN_PIN, HIGH); delay(300); digitalWrite(LED_SCAN_PIN, LOW);
    digitalWrite(LED_C1_PIN, HIGH);   delay(300); digitalWrite(LED_C1_PIN, LOW);
    digitalWrite(LED_C2_PIN, HIGH);   delay(300); digitalWrite(LED_C2_PIN, LOW);

    BLEDevice::init("PawlarDoor");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

// --- NEW FUNCTION: Check if we should disconnect ---
void checkProximity(BLEClient* client, bool &connectedFlag, int ledPin, String name) {
    if (client != nullptr && client->isConnected()) {
        // Read current signal strength
        int currentRSSI = client->getRssi();
        
        // Debug print (helps you calibrate!)
        // Serial.print(name); Serial.print(" RSSI: "); Serial.println(currentRSSI);

        // If signal is weaker than (THRESHOLD - BUFFER), Disconnect
        // Example: -60 - 5 = -65. If RSSI drops to -66, goodbye!
        if (currentRSSI < (RSSI_THRESHOLD - RSSI_BUFFER)) {
            Serial.print("👋 "); Serial.print(name); Serial.println(" moved away. Disconnecting.");
            client->disconnect();
            connectedFlag = false;
            digitalWrite(ledPin, LOW);
        }
    }
}

void loop() {
    // 1. SCANNING (If needed)
    if (!connectedC1 || !connectedC2) {
        digitalWrite(LED_SCAN_PIN, HIGH);
        pBLEScan->start(1, false); // Quick 1 sec scan
        pBLEScan->clearResults();
        digitalWrite(LED_SCAN_PIN, LOW);
    }

    // 2. CONNECT NEW DEVICES
    if (pAddressC1 != nullptr && !connectedC1) {
        if (connectToCollar(*pAddressC1, pClientC1)) {
            connectedC1 = true;
            digitalWrite(LED_C1_PIN, HIGH);
        }
        delete pAddressC1; pAddressC1 = nullptr;
    }

    if (pAddressC2 != nullptr && !connectedC2) {
        if (connectToCollar(*pAddressC2, pClientC2)) {
            connectedC2 = true;
            digitalWrite(LED_C2_PIN, HIGH);
        }
        delete pAddressC2; pAddressC2 = nullptr;
    }

    // 3. MONITOR DISTANCE (The 11-inch Rule)
    if (connectedC1) checkProximity(pClientC1, connectedC1, LED_C1_PIN, "C1");
    if (connectedC2) checkProximity(pClientC2, connectedC2, LED_C2_PIN, "C2");

    delay(200);
}