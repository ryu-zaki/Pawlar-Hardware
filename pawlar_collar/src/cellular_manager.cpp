#include <WiFi.h>
#include "cellular_manager.h"
#include "config.h"
#include "storage_manager.h"
#include "gps_manager.h"

// Using Hardware Serial 1 (Serial1) instead of SoftwareSerial for 100% stability
#define cellSerial Serial1

bool waitForResponse(const char* expected, uint32_t timeout) {
    uint32_t start = millis();
    while (millis() - start < timeout) {
        if (cellSerial.find((char*)expected)) return true; 
        
        // Instead of delay(10); feed the GPS so the buffer doesn't overflow!
        readGPS(); 
        delay(2); 
    }
    return false;
}

void initCellular() {
    // Pins 6 (RX) and 7 (TX) mapped to Serial1
    cellSerial.begin(GSM_BAUD, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
    Serial.println("📡 Initializing A7670C (Hardware Serial1)...");
    
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

    // --- ⏰ TIME SYNC (Critical for HiveMQ SSL) ---
    Serial.println("⏰ Syncing module time via NTP (this can take 5-10 seconds)...");
    
    // AT+CNTP="server",timezone_offset_in_quarter_hours
    // Example: Offset 32 means +8 hours (GTM+8). Change if you are in a different zone.
    cellSerial.println("AT+CNTP=\"pool.ntp.org\",32,1,20000"); 
    waitForResponse("OK", 2000);
    
    cellSerial.println("AT+CNTP"); // Start the sync process
    if (waitForResponse("+CNTP: 0", 25000)) {
        Serial.println("✅ Time Synced successfully!");
    } else {
        Serial.println("⚠️ Time Sync timed out. SSL handshake might still fail.");
        // Fallback: Check if the clock is at least set to something modern
        cellSerial.println("AT+CCLK?");
        waitForResponse("OK", 1000);
    }
}

// Shared helper for MQTT connection to HiveMQ
bool connectMQTTStack(String clientID) {
    // 1. Ensure fresh start ONLY if not already started
    cellSerial.println("AT+CMQTTSTART?");
    if (!waitForResponse("+CMQTTSTART: 1", 1000)) {
        Serial.println("🔄 Resetting MQTT Stack...");
        cellSerial.println("AT+CMQTTDISC=0,60"); delay(200);
        cellSerial.println("AT+CMQTTREL=0"); delay(200);
        cellSerial.println("AT+CMQTTSTOP"); delay(200);
        cellSerial.println("AT+CMQTTSTART");
        if (!waitForResponse("OK", 5000)) return false;
    }

    // 2. Ensure Network is Open
    cellSerial.println("AT+NETOPEN?");
    if (!waitForResponse("+NETOPEN: 1", 1000)) {
        cellSerial.println("AT+NETOPEN");
        if (!waitForResponse("+NETOPEN: 0", 15000)) return false;
        delay(2000); 
    }

    // 4. Configure SSL (Critical for HiveMQ Cloud)
    cellSerial.println("AT+CSSLCFG=\"sslversion\",0,4"); waitForResponse("OK", 1000);
    cellSerial.println("AT+CSSLCFG=\"authmode\",0,0"); waitForResponse("OK", 1000);
    cellSerial.println("AT+CSSLCFG=\"ignore_cert_exp\",0,1"); waitForResponse("OK", 1000); 
    cellSerial.println("AT+CSSLCFG=\"enableSNI\",0,1"); waitForResponse("OK", 1000);
    cellSerial.println("AT+CSSLCFG=\"sni\",0,\"" + String(MQTT_SERVER) + "\""); waitForResponse("OK", 1000);

    // 5. Release and Acquire Client
    cellSerial.println("AT+CMQTTREL=0"); delay(200); 
    cellSerial.println("AT+CMQTTACCQ=0,\"" + clientID + "\",1"); 
    if (!waitForResponse("OK", 5000)) return false;

    cellSerial.println("AT+CMQTTSSLCFG=0,0"); waitForResponse("OK", 1000);

// 6. Connect to Broker
    Serial.println("🔐 Connecting to HiveMQ SSL...");
    // Keep tcp:// prefix - it is confirmed working for this firmware
    String conn = "AT+CMQTTCONNECT=0,\"tcp://" + String(MQTT_SERVER) + ":8883\",60,1,\"" + String(MQTT_USER) + "\",\"" + String(MQTT_PASSWORD) + "\"";
    cellSerial.println(conn);
    
    // VERBOSE CAPTURE
    uint32_t start = millis();
    bool success = false;
    while (millis() - start < 45000) {
        
        // 🚨 SMART DELAY: Constantly scoop data out of the GPS buffer!
        readGPS(); 

        while (cellSerial.available()) {
            String line = cellSerial.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
                Serial.println("📡 MODEM: " + line);
                if (line.indexOf("+CMQTTCONNECT: 0,0") != -1) {
                    success = true;
                    goto end_capture;
                }
                if (line.indexOf("+CMQTTCONNECT:") != -1 && line.indexOf("0,0") == -1) {
                    success = false;
                    goto end_capture;
                }
            }
        }
        
        // We drop this from delay(10) down to delay(2) to keep the loop cycling 
        // fast enough to catch every single character the GPS spits out.
        delay(2); 
    }

end_capture:
    return success;
}

// Shared helper to clean up MQTT session
void disconnectMQTTStack() {
    delay(1000);
    cellSerial.println("AT+CMQTTDISC=0,120"); waitForResponse("OK", 2000);
    cellSerial.println("AT+CMQTTREL=0"); waitForResponse("OK", 2000);
    cellSerial.println("AT+CMQTTSTOP"); waitForResponse("OK", 2000);
}

// Check if MQTT is currently connected to the broker
bool isMQTTConnected() {
    cellSerial.println("AT+CMQTTCONNECT?");
    // Returns +CMQTTCONNECT: 0,"tcp://...",...
    // We look for the presence of the server address which indicates an active link
    return waitForResponse("+CMQTTCONNECT: 0,\"", 1000);
}

bool sendCellularMQTT(float lat, float lng, int bat, int sats, String status) {
    if (WiFi.status() == WL_CONNECTED) return false;

    // Only send if we have a valid fix to match the backend expectations
    if (status != "LOCKED") {
        Serial.println("🛰️ 4G: GPS Scanning (No Fix, skipping publish)");
        return false;
    }

    // Only connect if we aren't already
    if (!isMQTTConnected()) {
        Serial.println("\n🌍 4G: Establishing persistent link...");
        String clientID = "Collar-GPS-" + getMACAddress();
        if (!connectMQTTStack(clientID)) {
            Serial.println("❌ 4G: Connection Failed.");
            return false;
        }
        Serial.println("✅ 4G: Link Established.");
    }
    
    // Corrected payload format for map display
    String payload = "{\"device_id\":\"" + getUniqueDeviceID() + "\",\"coords\":{\"lat\":" + String(lat, 6) + ",\"long\":" + String(lng, 6) + "}}";
    String topic = String(TOPIC_GPS_PUB);

    cellSerial.println("AT+CMQTTTOPIC=0," + String(topic.length()));
    delay(100); cellSerial.print(topic); delay(100);
    
    cellSerial.println("AT+CMQTTPAYLOAD=0," + String(payload.length()));
    delay(100); cellSerial.print(payload); delay(100);
    
    cellSerial.println("AT+CMQTTPUB=0,1,60");
    
    if (waitForResponse("+CMQTTPUB: 0,0", 5000)) {
        Serial.println("📤 4G: GPS Sent (LOCKED)");
        return true;
    } else {
        Serial.println("⚠️ 4G: Publish Failed.");
        return false;
    }
}

bool sendWifiStatusCellular(bool isConnected) {
    if (WiFi.status() == WL_CONNECTED && isConnected) return false;
    Serial.println("\n🌍 4G FAILOVER: Sending WiFi Status...");

    String macAddr = getMACAddress();
    String clientID = "Collar-WiFi-" + macAddr;
    
    if (connectMQTTStack(clientID)) {
        String payload = "{\"device_id\":\"" + macAddr + "\",\"isConnected\":" + (isConnected ? "true" : "false") + "}";
        String topic = String(TOPIC_WIFI_PUB) + "/" + macAddr;

        cellSerial.println("AT+CMQTTTOPIC=0," + String(topic.length()));
        delay(100); cellSerial.print(topic); delay(100);
        
        cellSerial.println("AT+CMQTTPAYLOAD=0," + String(payload.length()));
        delay(100); cellSerial.print(payload); delay(100);
        
        Serial.println("📤 Publishing WiFi Status...");
        cellSerial.println("AT+CMQTTPUB=0,1,60");
        
        if (waitForResponse("+CMQTTPUB: 0,0", 10000)) {
            Serial.println("✅ WiFi Status Published!");
        }

        disconnectMQTTStack();
        return true;
    }
    return false;
}