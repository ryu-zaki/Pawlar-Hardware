#include <Preferences.h>
#include "storage_manager.h"
#include <nvs_flash.h>

Preferences prefs;

void saveCredentials(String ssid, String pass) {
    if (prefs.begin("pawlar_f", false)) {
        prefs.putString("ssid", ssid);
        prefs.putString("pass", pass);
        prefs.end();
    }
}

String getSSID() {
    String s = "";
    if (prefs.begin("pawlar_f", true)) {
        s = prefs.getString("ssid", ""); 
        prefs.end();
    }
    return s;
}

String getPass() {
    String p = "";
    if (prefs.begin("pawlar_f", true)) {
        p = prefs.getString("pass", ""); 
        prefs.end();
    }
    return p;
}

String getDeviceId() {
    String id = "";
    if (prefs.begin("pawlar_f", false)) {
        if (prefs.isKey("device_id")) {
            id = prefs.getString("device_id", "");
        } else {
            id = getUniqueFeederID();
            prefs.putString("device_id", id);
            Serial.println("🆔 Generated and saved new Device ID: " + id);
        }
        prefs.end();
    }
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
    if (prefs.begin("pawlar_f", false)) {
        prefs.putString("collar_list", collarList);
        prefs.end();
    }
}

String getAuthorizedCollarList() {
    String list = "";
    if (prefs.begin("pawlar_f", true)) {
        list = prefs.getString("collar_list", ""); 
        prefs.end();
    }
    return list; 
}

void saveGramsPerServing(float grams) {
    if (prefs.begin("pawlar_f", false)) {
        prefs.putFloat("grams_per", grams);
        prefs.end();
    }
}

float getGramsPerServing() {
    float grams = 20.0f;
    if (prefs.begin("pawlar_f", true)) {
        grams = prefs.getFloat("grams_per", 20.0f);
        prefs.end();
    }
    return grams;
}

bool isNewlyRegistered() {
    bool b = false;
    if (prefs.begin("pawlar_f", true)) {
        b = prefs.getBool("new_reg", false);
        prefs.end();
    }
    return b;
}

void setNewlyRegistered(bool b) {
    if (prefs.begin("pawlar_f", false)) {
        prefs.putBool("new_reg", b);
        prefs.end();
    }
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
