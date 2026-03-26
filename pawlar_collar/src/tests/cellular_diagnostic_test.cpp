#include <Arduino.h>
#include <SoftwareSerial.h>

// --- Configuration (Matching your wiring) ---
#define GSM_RX_PIN 6  // A7670C TX
#define GSM_TX_PIN 7  // A7670C RX
#define GSM_PWR_KEY 5 // Software Power Toggle
#define GSM_BAUD 115200

// Use Hardware Serial 1
#define cellSerial Serial1 

bool waitForResponse(const char* expected, uint32_t timeout) {
    uint32_t start = millis();
    while (millis() - start < timeout) {
        if (cellSerial.find((char*)expected)) return true;
        delay(10);
    }
    return false;
}

void sendAT(const char* cmd, const char* label) {
    Serial.printf("\n--- [%s] ---\n", label);
    Serial.printf("Command: %s\n", cmd);
    cellSerial.println(cmd);
    
    uint32_t start = millis();
    bool found = false;
    while (millis() - start < 5000) {
        while (cellSerial.available()) {
            char c = cellSerial.read();
            Serial.print(c);
            found = true;
        }
        delay(10);
    }
    if (!found) Serial.println(" (No response)");
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n🔍 PAWLAR CELLULAR DIAGNOSTIC (HARDWARE SERIAL) 🔍");
    
    // Initialize Hardware Serial 1 on Pins 6 (RX) and 7 (TX)
    cellSerial.begin(GSM_BAUD, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
    
    // 1. Power On Pulse
    Serial.println("⚡ Pulsing PWR_KEY (Pin 5)...");
    pinMode(GSM_PWR_KEY, OUTPUT);
    digitalWrite(GSM_PWR_KEY, HIGH); delay(1500);
    digitalWrite(GSM_PWR_KEY, LOW);
    delay(5000); // Wait for boot

    // 2. Basic Communication
    sendAT("AT", "Basic Communication Check");
    
    // 3. Module Info
    sendAT("ATI", "Module Information");
    
    // 4. SIM Status
    sendAT("AT+CPIN?", "SIM Status");
    
    // 5. Signal Strength
    sendAT("AT+CSQ", "Signal Strength (10-31 is good)");
    
    // 6. Network Registration
    sendAT("AT+CREG?", "Network Registration (0,1 or 0,5 is success)");
    sendAT("AT+CGREG?", "GPRS Registration");
    
    // 7. Preferred Mode (Force LTE/4G if needed)
    sendAT("AT+CNMP?", "Preferred Network Mode");
    
    // 8. APN Check
    sendAT("AT+CGDCONT?", "APN Configuration");
    
    // 9. Network Open Check
    sendAT("AT+NETOPEN?", "Check if Network is Open");
    
    Serial.println("\n--- 🌐 ATTEMPTING INTERNET CONNECTION ---");
    cellSerial.println("AT+NETOPEN");
    delay(3000);
    while(cellSerial.available()) Serial.print((char)cellSerial.read());
    
    // 10. Ping Google DNS
    sendAT("AT+CPING=\"8.8.8.8\",1,4", "Pinging Google DNS (8.8.8.8)");
    
    Serial.println("\n--- ☁️ MQTT STACK CHECK ---");
    sendAT("AT+CMQTTSTART", "Starting MQTT Service");
    sendAT("AT+CMQTTACCQ=0,\"TestClient\",0", "Acquiring Client (No SSL)");
    
    Serial.println("\n✅ DIAGNOSTIC COMPLETE.");
    Serial.println("If you don't see 'OK' for AT+NETOPEN or +CPING success, data is not working.");
}

void loop() {
    // Manual Tunnel if user wants to type more commands
    while (cellSerial.available()) Serial.print((char)cellSerial.read());
    while (Serial.available()) cellSerial.write(Serial.read());
}
