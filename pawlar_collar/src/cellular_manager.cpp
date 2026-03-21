#include <WiFi.h>
#include <SoftwareSerial.h>
#include "cellular_manager.h"
#include "config.h"
#include "storage_manager.h"

// Software Serial on Pins 20 and 21 to avoid HardwareSerial conflict
SoftwareSerial cellSerial(GSM_RX_PIN, GSM_TX_PIN);

bool waitForResponse(const char* expected, uint32_t timeout) {
    uint32_t start = millis();
    while (millis() - start < timeout) {
        if (cellSerial.find((char*)expected)) return true; // Added explicit cast for find
        delay(10); // CRITICAL: Yields to system to prevent WDT reset
    }
    return false;
}

void initCellular() {
    cellSerial.begin(GSM_BAUD);
    Serial.println("📡 Checking if A7670C is already awake...");
    
    // Check 3 times if it's already on
    bool alreadyOn = false;
    for(int i=0; i<3; i++) {
        cellSerial.println("AT"); 
        if (waitForResponse("OK", 500)) {
            alreadyOn = true;
            break;
        }
        delay(200);
    }

    if (alreadyOn) {
        Serial.println("✅ A7670C is already ON and responding!");
    } else {
        Serial.println("⚡ Module silent. Pulsing PWR_KEY (Pin 5) to wake it up...");
        pinMode(5, OUTPUT); 
        digitalWrite(5, HIGH); delay(1500); // Standard pulse
        digitalWrite(5, LOW);
        delay(3000); // Wait for boot
        
        cellSerial.println("AT");
        if (waitForResponse("OK", 2000)) {
            Serial.println("✅ A7670C successfully woken up!");
        } else {
            Serial.println("⚠️ Still no response. Check PEN pin and 5V Power supply!");
            return;
        }
    }
    
    // Basic setup
    cellSerial.println("AT+CGDCONT=1,\"IP\",\"internet\""); 
    waitForResponse("OK", 2000);

    // --- 🔍 NETWORK DIAGNOSTICS ---
    Serial.println("📡 Checking Network Status...");
    
    // Check Signal Strength (CSQ)
    cellSerial.println("AT+CSQ");
    waitForResponse("+CSQ:", 1000); 
    
    // Check Network Registration (CREG)
    cellSerial.println("AT+CREG?");
    if (waitForResponse("+CREG: 0,1", 1000) || waitForResponse("+CREG: 0,5", 1000)) {
        Serial.println("✅ NETWORK: Registered (Home or Roaming)");
    } else {
        Serial.println("❌ NETWORK: Not Registered. Check SIM or Antenna!");
    }

    Serial.println("🌐 Cellular ready for failover.");
}

bool sendCellularMQTT(float lat, float lng, int bat) {
    if (WiFi.status() == WL_CONNECTED) return false;

    Serial.println("\n🌍 4G FAILOVER: SSL Connection Attempt");

    cellSerial.println("AT+NETOPEN?");
    if (!waitForResponse("+NETOPEN: 1", 1000)) {
        cellSerial.println("AT+NETOPEN");
        waitForResponse("+NETOPEN: 0", 5000);
    }

    cellSerial.println("AT+CMQTTSTOP");
    delay(500);
    cellSerial.println("AT+CMQTTREL=0");
    delay(500);

    cellSerial.println("AT+CSSLCFG=\"sslversion\",0,4"); 
    waitForResponse("OK", 1000);
    cellSerial.println("AT+CSSLCFG=\"authmode\",0,0"); 
    waitForResponse("OK", 1000);
    cellSerial.println("AT+CSSLCFG=\"enableSNI\",0,1"); 
    waitForResponse("OK", 1000);

    cellSerial.println("AT+CMQTTSTART");
    if (!waitForResponse("OK", 5000)) {
        Serial.println("❌ MQTT START ERROR");
        return false;
    }

    String macAddr = getMACAddress();
    String clientID = "Collar-" + macAddr;
    cellSerial.println("AT+CMQTTACCQ=0,\"" + clientID + "\",1"); 
    if (!waitForResponse("OK", 5000)) return false;

    cellSerial.println("AT+CMQTTSSLCFG=0,0"); 
    waitForResponse("OK", 1000);

    Serial.println("🔐 Connecting to HiveMQ SSL...");
    // Properly formatted connection string
    String conn = "AT+CMQTTCONNECT=0,\"ssl://" + String(MQTT_SERVER) + ":8883\",60,1,\"" + String(MQTT_USER) + "\",\"" + String(MQTT_PASSWORD) + "\"";
    cellSerial.println(conn);
    
    if (waitForResponse("+CMQTTCONNECT: 0,0", 45000)) { 
        Serial.println("✅ 4G MQTT SUCCESS!");
        
        String payload = "{\"id\":\"" + getUniqueDeviceID() + "\",\"lat\":" + String(lat, 6) + ",\"lng\":" + String(lng, 6) + ",\"bat\":" + String(bat) + "}";
        cellSerial.println("AT+CMQTTTOPIC=0," + String(String(TOPIC_GPS_PUB).length()));
        delay(100); cellSerial.println(TOPIC_GPS_PUB);
        cellSerial.println("AT+CMQTTPAYLOAD=0," + String(payload.length()));
        delay(100); cellSerial.println(payload);
        cellSerial.println("AT+CMQTTPUB=0,1,60");
        return true;
    } else {
        Serial.println("❌ SSL Handshake Failed.");
        cellSerial.println("AT+CMQTTCONNERR?"); 
        return false;
    }
}

bool sendWifiStatusCellular(bool isConnected) {
    if (WiFi.status() == WL_CONNECTED && isConnected) return false;

    Serial.println("\n🌍 4G: Sending WiFi Status...");

    cellSerial.println("AT+NETOPEN?");
    if (!waitForResponse("+NETOPEN: 1", 1000)) {
        cellSerial.println("AT+NETOPEN");
        waitForResponse("+NETOPEN: 0", 5000);
    }

    cellSerial.println("AT+CMQTTSTOP");
    delay(500);
    cellSerial.println("AT+CMQTTREL=0");
    delay(500);

    cellSerial.println("AT+CMQTTSTART");
    if (!waitForResponse("OK", 5000)) return false;

    String macAddr = getMACAddress();
    String clientID = "Collar-WiFi-" + macAddr;
    cellSerial.println("AT+CMQTTACCQ=0,\"" + clientID + "\",1"); 
    if (!waitForResponse("OK", 5000)) return false;

    cellSerial.println("AT+CMQTTSSLCFG=0,0"); 
    waitForResponse("OK", 1000);

    String conn = "AT+CMQTTCONNECT=0,\"ssl://" + String(MQTT_SERVER) + ":8883\",60,1,\"" + String(MQTT_USER) + "\",\"" + String(MQTT_PASSWORD) + "\"";
    cellSerial.println(conn);
    
    if (waitForResponse("+CMQTTCONNECT: 0,0", 45000)) { 
        String payload = "{\"device_id\":\"" + macAddr + "\",\"isConnected\":" + (isConnected ? "true" : "false") + "}";
        String topic = String(TOPIC_WIFI_PUB) + "/" + macAddr;

        cellSerial.println("AT+CMQTTTOPIC=0," + String(topic.length()));
        delay(100); cellSerial.println(topic);
        cellSerial.println("AT+CMQTTPAYLOAD=0," + String(payload.length()));
        delay(100); cellSerial.println(payload);
        cellSerial.println("AT+CMQTTPUB=0,1,60");
        
        Serial.println("✅ WiFi Status sent via 4G: " + payload);
        return true;
    }
    return false;
}