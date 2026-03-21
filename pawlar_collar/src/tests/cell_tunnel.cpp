#include <Arduino.h>
#include <SoftwareSerial.h>

// --- Configuration ---
#define GSM_RX_PIN 6  // Connect A7670C TX here
#define GSM_TX_PIN 7  // Connect A7670C RX here
#define GSM_BAUD 115200

SoftwareSerial cellSerial(GSM_RX_PIN, GSM_TX_PIN);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n--- 📡 CELLULAR RAW TUNNEL (DEBUG) ---");
  Serial.println("Goal: Type 'AT' and see 'OK' on the screen.");
  Serial.printf("Config: ESP32 Pin %d (RX) <--- A7670C TX\n", GSM_RX_PIN);
  Serial.printf("Config: ESP32 Pin %d (TX) ---> A7670C RX\n", GSM_TX_PIN);
  
  cellSerial.begin(GSM_BAUD);
  
  Serial.println("Tunnel Active. Type commands below (Ensure NL & CR is selected)...\n");
}

void loop() {
  // Pass from A7670C to USB
  while (cellSerial.available()) {
    char c = cellSerial.read();
    Serial.print(c);
  }

  // Pass from USB to A7670C
  while (Serial.available()) {
    char c = Serial.read();
    cellSerial.write(c);
  }
}
