#include <Preferences.h>
#include "storage_manager.h"
#include <nvs_flash.h>

Preferences prefs;

// --- 📶 WIFI CREDENTIALS ---
void saveCredentials(String ssid, String pass) {
    prefs.begin("pawlar", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
}

String getSSID() {
    prefs.begin("pawlar", true);
    String s = prefs.getString("ssid", ""); 
    prefs.end();
    return s;
}

String getPass() {
    prefs.begin("pawlar", true);
    String p = prefs.getString("pass", ""); 
    prefs.end();
    return p;
}

// --- 🆔 DEVICE & USER INFO (Bago para sa Flowchart) ---
void saveDeviceInfo(String deviceId, String userId) {
    prefs.begin("pawlar", false);
    prefs.putString("device_id", deviceId);
    prefs.putString("user_id", userId);
    prefs.end();
}

// --- 🆔 USER INFO ---
String getUserId() {
    prefs.begin("pawlar", true);
    // Default to "123" for your backend test if nothing is saved
    String u = prefs.getString("user_id", "123"); 
    prefs.end();
    return u;
}

String getDeviceId() {
    prefs.begin("pawlar", true); // Read-only mode
    String id = "";
    if (prefs.isKey("device_id")) {
        id = prefs.getString("device_id");
    }
    
    prefs.end();
    if (id == "") {
        return getUniqueDoorID();
    }
    return id;
}

String getUniqueDoorID() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
             
    return String(macStr);
}

// --- 🔒 AUTHORIZED COLLARS ---
void saveAuthorizedCollar(String collarList) {
    collarList.trim();
    prefs.begin("pawlar", false);
    prefs.putString("collar_list", collarList);
    prefs.end();
}

String getAuthorizedCollarList() {
    prefs.begin("pawlar", true);
    String list = "";
    
    // 🚩 FIX: Check if the key exists before trying to read it!
    if (prefs.isKey("collar_list")) {
        list = prefs.getString("collar_list", ""); 
    }
    
    prefs.end();
    list.trim();
    return list; 
}

// --- 🛠️ INITIALIZATION ---
void clearStorage() {
    Serial.println("⚠️ FACTORY RESET: Erasing all NVS data...");
    nvs_flash_erase();
    nvs_flash_init();
    Serial.println("✅ NVS Erased. Rebooting...");
    delay(2000);
    ESP.restart();
}

void initStorage() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    if (prefs.begin("pawlar", false)) {
        Serial.println("✅ NVS Storage Initialized successfully.");
        prefs.end();
    } else {
        Serial.println("❌ Critical Error: Could not open Preferences.");
    }
}