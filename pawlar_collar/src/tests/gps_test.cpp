#include <Arduino.h>
#include <TinyGPSPlus.h>

// --- Configuration ---
#define GPS_RX_PIN 3  // Connected to GPS TX
#define GPS_TX_PIN 2  // Connected to GPS RX
#define BUTTON_PIN 9  
#define GPS_BAUD 9600 // Try 9600 first, then 115200 if it fails

TinyGPSPlus gps;
// Using Hardware Serial 1 (more stable than SoftwareSerial on ESP32)
HardwareSerial gpsSerial(1);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n--- 🛠️ GPS ADVANCED DIAGNOSTIC ---");
  Serial.printf("ESP32-C3 RX: Pin %d <--- GPS TX\n", GPS_RX_PIN);
  Serial.printf("ESP32-C3 TX: Pin %d ---> GPS RX\n", GPS_TX_PIN);
  
  // Initialize Hardware Serial 1 on Pins 3 and 2
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("Monitoring for raw NMEA data...\n");
}

void loop() {
  // 1. RAW DATA CHECK (Pass-through)
  // If you see symbols like '$GPRMC' or even '', the connection is working.
  // If the screen stays empty, the wires are likely swapped or the GPS has no power.
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    Serial.write(c); // Print raw character to USB Serial
    gps.encode(c);   // Feed to parser
  }

  // 2. Periodic Summary (Every 5 seconds)
  static unsigned long lastSummary = 0;
  if (millis() - lastSummary > 5000) {
    Serial.println("\n\n--- 🛰️ 5s STATUS REPORT ---");
    Serial.printf("Characters Processed: %lu\n", (unsigned long)gps.charsProcessed());
    Serial.printf("Checksum Errors: %lu\n", (unsigned long)gps.failedChecksum());
    
    if (gps.charsProcessed() == 0) {
      Serial.println("❌ ERROR: No characters received. Check wires/power.");
      Serial.println("👉 ACTION: Try swapping Pin 3 and Pin 2 in the code or on the board.");
    } else if (gps.satellites.value() == 0) {
      Serial.println("⏳ STATUS: Receiving data, but NO SATELLITE FIX yet.");
      Serial.println("👉 ACTION: Go outside or near a window. Wait 2-5 minutes.");
    } else {
      Serial.printf("✅ FIX FOUND! Satellites: %d | Lat: %.6f | Lng: %.6f\n", 
                    gps.satellites.value(), gps.location.lat(), gps.location.lng());
    }
    Serial.println("---------------------------\n");
    lastSummary = millis();
  }

  // 3. Button Check
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("\n[Button Pressed] Resetting Serial at 115200 Baud just in case...");
    gpsSerial.begin(115200, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    delay(500);
    while(digitalRead(BUTTON_PIN) == LOW);
  }
}
