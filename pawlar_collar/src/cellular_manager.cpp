#include <WiFi.h>
#include "cellular_manager.h"
#include "config.h"
#include "storage_manager.h"

// Use HardwareSerial for the cellular module
HardwareSerial cellSerial(1);

bool waitForResponse(const char* expected, uint32_t timeout) {
    uint32_t start = millis();
    while (millis() - start < timeout) {
        if (cellSerial.find((char*)expected)) return true; // Added explicit cast for find
        delay(10); // CRITICAL: Yields to system to prevent WDT reset
    }
    return false;
}

void initCellular() {
    pinMode(5, OUTPUT); 
    digitalWrite(5, LOW); delay(100); 
    digitalWrite(5, HIGH); delay(1000);           
    digitalWrite(5, LOW);
    
    cellSerial.begin(GSM_BAUD, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
    
    Serial.println("📡 Checking A7670C Presence...");
    cellSerial.println("AT"); 
    
    // Lower the timeout to 500ms for the first check
    if (!waitForResponse("OK", 500)) { 
        Serial.println("⚠️ No Cellular Module detected. Skipping init.");
        return; // Exit function so it doesn't hang the loop
    }
    
    cellSerial.println("AT+CGDCONT=1,\"IP\",\"internet\""); 
    waitForResponse("OK", 2000);
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