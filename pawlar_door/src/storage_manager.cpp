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
    prefs.begin("pawlar", true);
    String id = prefs.getString("device_id", "DOOR_DEFAULT_ID"); // Give it a default
    prefs.end();
    return id;
}

String getUniqueDoorID() {
    uint64_t chipid = ESP.getEfuseMac(); 
    uint32_t low = (uint32_t)chipid;
    char idString[20];
    snprintf(idString, 20, "DOOR_%08X", low);
    return String(idString);
}

// --- 🔒 AUTHORIZED COLLARS ---
void saveAuthorizedCollar(String collarList) {
    prefs.begin("pawlar", false);
    prefs.putString("collar_list", collarList);
    prefs.end();
}

String getAuthorizedCollarList() {
    prefs.begin("pawlar", true);
    String list = prefs.getString("collar_list", ""); 
    prefs.end();
    return list; 
}

// --- 🛠️ INITIALIZATION ---
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