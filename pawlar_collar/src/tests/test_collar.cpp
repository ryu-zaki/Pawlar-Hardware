/**
 * @file test_collar.cpp
 * @brief Logic for the Collar (BLE Peripheral)
 * * LEDs:
 * - RED (Pin 1): Inactive / Advertising / Not Connected
 * - GREEN (Pin 0): Connected to Door
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// --- ⚙️ CONFIGURATION ---
// CHANGE THIS TO "PAWLAR_C2" WHEN UPLOADING TO THE SECOND COLLAR!
#define DEVICE_NAME "PAWLAR_C1" 

// Pins (ESP32-C3 Mini)
#define LED_CONNECTED_PIN 0  // Green
#define LED_INACTIVE_PIN  1  // Red

// UUIDs (Must match the Door code)
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// --- CALLBACKS ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("✅ Door Connected!");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("❌ Door Disconnected.");
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("🐶 Collar Starting: " DEVICE_NAME);

  pinMode(LED_CONNECTED_PIN, OUTPUT);
  pinMode(LED_INACTIVE_PIN, OUTPUT);

  // 1. Initialize BLE
  BLEDevice::init(DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 2. Create Service & Characteristic
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pService->start();

  // 3. Start Advertising (So the door can find us)
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); 
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("📡 Advertising started...");
}

void loop() {
    // --- LED LOGIC ---
    if (deviceConnected) {
        digitalWrite(LED_CONNECTED_PIN, HIGH); // Green ON
        digitalWrite(LED_INACTIVE_PIN, LOW);   // Red OFF
    } else {
        digitalWrite(LED_CONNECTED_PIN, LOW);  // Green OFF
        digitalWrite(LED_INACTIVE_PIN, HIGH);  // Red ON
    }

    // --- RE-ADVERTISING LOGIC ---
    // If connection drops, we must start advertising again to be found
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Give bluetooth stack a chance to get ready
        pServer->startAdvertising(); 
        Serial.println("📡 Restarting Advertising...");
        oldDeviceConnected = deviceConnected;
    }
    
    // Connection handling updates
    if (deviceConnected && !oldDeviceConnected) {
        // Just connected
        oldDeviceConnected = deviceConnected;
    }
    
    delay(100);
}