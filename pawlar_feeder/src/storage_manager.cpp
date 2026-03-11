#include <Preferences.h>
#include "storage_manager.h"
#include <nvs_flash.h>

Preferences prefs;

void saveCredentials(String ssid, String pass) {
    prefs.begin("pawlar_f", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
}

String getSSID() {
    prefs.begin("pawlar_f", true);
    String s = prefs.getString("ssid", ""); 
    prefs.end();
    return s;
}

String getPass() {
    prefs.begin("pawlar_f", true);
    String p = prefs.getString("pass", ""); 
    prefs.end();
    return p;
}

String getDeviceId() {
    prefs.begin("pawlar_f", true);
    String id = prefs.getString("device_id", "");
    prefs.end();
    if (id == "") return getUniqueFeederID();
    return id;
}

String getUniqueFeederID() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

void saveAuthorizedCollar(String collarList) {
    collarList.trim();
    prefs.begin("pawlar_f", false);
    prefs.putString("collar_list", collarList);
    prefs.end();
}

String getAuthorizedCollarList() {
    prefs.begin("pawlar_f", true);
    String list = prefs.getString("collar_list", ""); 
    prefs.end();
    return list; 
}

void clearStorage() {
    nvs_flash_erase();
    nvs_flash_init();
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
}
