#include <WiFi.h>
#include <SoftwareSerial.h> 
#include "cellular_manager.h"
#include "config.h"
#include "storage_manager.h"

// Initialize SoftwareSerial on pins defined in config.h
SoftwareSerial swSerial(GSM_RX_PIN, GSM_TX_PIN); 

bool waitForResponse(const char* expected, uint32_t timeout) {
    uint32_t start = millis();
    while (millis() - start < timeout) {
        if (swSerial.find((char*)expected)) return true; // Added explicit cast for find
        delay(10); // CRITICAL: Yields to system to prevent WDT reset
    }
    return false;
}

void initCellular() {
    pinMode(5, OUTPUT); 
    digitalWrite(5, LOW); delay(100); 
    digitalWrite(5, HIGH); delay(1000);           
    digitalWrite(5, LOW);
    
    swSerial.begin(GSM_BAUD);
    
    Serial.println("📡 Checking A7670C Presence...");
    swSerial.println("AT"); 
    
    // Lower the timeout to 500ms for the first check
    if (!waitForResponse("OK", 500)) { 
        Serial.println("⚠️ No Cellular Module detected. Skipping init.");
        return; // Exit function so it doesn't hang the loop
    }
    
    swSerial.println("AT+CGDCONT=1,\"IP\",\"internet\""); 
    waitForResponse("OK", 2000);
}

bool sendCellularMQTT(float lat, float lng, int bat) {
    if (WiFi.status() == WL_CONNECTED) return false;

    Serial.println("\n🌍 4G FAILOVER: SSL Connection Attempt");

    swSerial.println("AT+NETOPEN?");
    if (!waitForResponse("+NETOPEN: 1", 1000)) {
        swSerial.println("AT+NETOPEN");
        waitForResponse("+NETOPEN: 0", 5000);
    }

    swSerial.println("AT+CMQTTSTOP");
    delay(500);
    swSerial.println("AT+CMQTTREL=0");
    delay(500);

    swSerial.println("AT+CSSLCFG=\"sslversion\",0,4"); 
    waitForResponse("OK", 1000);
    swSerial.println("AT+CSSLCFG=\"authmode\",0,0"); 
    waitForResponse("OK", 1000);
    swSerial.println("AT+CSSLCFG=\"enableSNI\",0,1"); 
    waitForResponse("OK", 1000);

    swSerial.println("AT+CMQTTSTART");
    if (!waitForResponse("OK", 5000)) {
        Serial.println("❌ MQTT START ERROR");
        return false;
    }

    String clientID = "Collar-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    swSerial.println("AT+CMQTTACCQ=0,\"" + clientID + "\",1"); 
    if (!waitForResponse("OK", 5000)) return false;

    swSerial.println("AT+CMQTTSSLCFG=0,0"); 
    waitForResponse("OK", 1000);

    Serial.println("🔐 Connecting to HiveMQ SSL...");
    // Properly formatted connection string
    String conn = "AT+CMQTTCONNECT=0,\"ssl://" + String(MQTT_SERVER) + ":8883\",60,1,\"" + String(MQTT_USER) + "\",\"" + String(MQTT_PASSWORD) + "\"";
    swSerial.println(conn);
    
    if (waitForResponse("+CMQTTCONNECT: 0,0", 45000)) { 
        Serial.println("✅ 4G MQTT SUCCESS!");
        
        String payload = "{\"id\":\"" + getUniqueDeviceID() + "\",\"lat\":" + String(lat, 6) + ",\"lng\":" + String(lng, 6) + ",\"bat\":" + String(bat) + "}";
        swSerial.println("AT+CMQTTTOPIC=0," + String(String(TOPIC_GPS_PUB).length()));
        delay(100); swSerial.println(TOPIC_GPS_PUB);
        swSerial.println("AT+CMQTTPAYLOAD=0," + String(payload.length()));
        delay(100); swSerial.println(payload);
        swSerial.println("AT+CMQTTPUB=0,1,60");
        return true;
    } else {
        Serial.println("❌ SSL Handshake Failed.");
        swSerial.println("AT+CMQTTCONNERR?"); 
        return false;
    }
}