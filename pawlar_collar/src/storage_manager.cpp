#include "storage_manager.h"
#include <Preferences.h>
#include <nvs_flash.h>
Preferences preferences;

void saveWiFiCreds(String ssid, String pass) {
    preferences.begin("wifi_creds", false);
    preferences.putString("ssid", ssid);
    preferences.putString("pass", pass);
    preferences.end();
}
String getSSID() {
    preferences.begin("wifi_creds", true);
    String s = preferences.getString("ssid", "");
    preferences.end();
    return s;
}
String getPass() {
    preferences.begin("wifi_creds", true);
    String p = preferences.getString("pass", "");
    preferences.end();
    return p;
}
bool isPairingRequested() {
    preferences.begin("wifi_creds", true);
    bool b = preferences.getBool("pairing_req", false);
    preferences.end();
    return b;
}
void setPairingRequest(bool enable) {
    preferences.begin("wifi_creds", false);
    preferences.putBool("pairing_req", enable);
    preferences.end();
}
String getMACAddress() {
    uint64_t chipid = ESP.getEfuseMac();
    char macStr[20];
    snprintf(macStr, 20, "%02X:%02X:%02X:%02X:%02X:%02X",
             (uint8_t)(chipid >> 0),
             (uint8_t)(chipid >> 8),
             (uint8_t)(chipid >> 16),
             (uint8_t)(chipid >> 24),
             (uint8_t)(chipid >> 32),
             (uint8_t)(chipid >> 40));
    return String(macStr);
}
String getUniqueDeviceID() {
    uint64_t chipid = ESP.getEfuseMac();
    uint32_t low = (uint32_t)chipid;
    char idString[20];
    snprintf(idString, 20, "COLLAR_%08X", low);
    return String(idString);
}
void initStorage() {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    
    // If NVS is full or has a new version, erase it and re-init
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    Serial.println("✅ NVS Storage Initialized");
}