#include "storage_manager.h"
#include <Preferences.h>
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
String getUniqueDeviceID() {
    uint64_t chipid = ESP.getEfuseMac();
    uint32_t low = (uint32_t)chipid;
    char idString[20];
    snprintf(idString, 20, "COLLAR_%08X", low);
    return String(idString);
}