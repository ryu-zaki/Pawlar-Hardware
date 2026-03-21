#include <Arduino.h>
#include <SoftwareSerial.h>

// LOOPBACK TEST: Connect a wire between Pin 2 and Pin 3
// We are using explicit GPIO numbers now
#define TX_PIN 3
#define RX_PIN 2

SoftwareSerial loopback; 

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // Explicitly assign pins to SoftwareSerial
    loopback.begin(9600, SWSERIAL_8N1, RX_PIN, TX_PIN);
    
    Serial.println("\n--- 🔄 ESP32-C3 GPIO LOOPBACK TEST ---");
    Serial.printf("Connecting GPIO %d to GPIO %d\n", TX_PIN, RX_PIN);
    Serial.println("Ensure your jumper wire is on the pins labeled '2' and '3'!");
}

void loop() {
    // Send a message out of Pin 3
    loopback.println("HELLO_WORLD");
    delay(100);

    // Try to read it back in from Pin 2
    if (loopback.available()) {
        String response = loopback.readStringUntil('\n');
        Serial.println("✅ SUCCESS! Received: " + response);
    } else {
        Serial.println("❌ FAILED: No data received. Check your jumper wire!");
    }
    delay(2000);
}
