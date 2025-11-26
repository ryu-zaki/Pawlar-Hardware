#include "gps_manager.h"
#include "config.h"
#include <TinyGPSPlus.h>
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

void initGPS() { gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN); }
void readGPS() { while (gpsSerial.available() > 0) gps.encode(gpsSerial.read()); }
bool hasFix() { return gps.location.isValid(); }
float getLat() { return gps.location.lat(); }
float getLng() { return gps.location.lng(); }
int getSatellites() { return gps.satellites.value(); }