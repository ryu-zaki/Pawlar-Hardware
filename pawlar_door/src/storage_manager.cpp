#include <Preferences.h>
#include "storage_manager.h"

Preferences prefs;

void saveCredentials(String ssid, String pass) {
    prefs.begin("wifi-gate", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
}

String getSSID() {
    prefs.begin("wifi-gate", true);
    String s = prefs.getString("ssid", "");
    prefs.end();
    return s;
}

String getPass() {
    prefs.begin("wifi-gate", true);
    String p = prefs.getString("pass", "");
    prefs.end();
    return p;
}

void saveAuthorizedCollar(String collarID) {
    prefs.begin("pawlar-auth", false);
    prefs.putString("collar_id", collarID);
    prefs.end();
}

String getAuthorizedCollar() {
    prefs.begin("pawlar-auth", true);
    String id = prefs.getString("collar_id", ""); // Returns empty if none registered
    prefs.end();
    return id;
}

void initStorage() {
    Preferences preferences;
    preferences.begin("pawlar", false); 
    preferences.end();
    Serial.println("💾 NVS Storage Initialized");
}