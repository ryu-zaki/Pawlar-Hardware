#include "storage_manager.h"
#include <Preferences.h>
#include <nvs_flash.h>
#include <esp_system.h>

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
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    char macStr[20];
    snprintf(macStr, 20, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}
String getUniqueDeviceID() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    char idString[20];
    snprintf(idString, 20, "COLLAR_%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
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