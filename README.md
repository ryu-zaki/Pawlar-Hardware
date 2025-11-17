# Pawlar Hardware Ecosystem 🐾

This documentation outlines the logic, architecture, and communication protocols for the Pawlar smart hardware components: **Smart Collar, Smart Feeder, and Smart Door**.

These devices communicate with the Pawlar Node.js/Express Backend to provide real-time data to the React Native/Capacitor Frontend.

---

## 🛠 Hardware Tech Stack (Recommended)

- **Microcontroller:** ESP32 (Recommended for WiFi/Bluetooth capabilities)
- **Communication Protocol:** - **HTTP/REST:** For sending logs and fetching schedules (Easiest integration with current backend).
  - **MQTT (Optional):** For real-time status updates (e.g., GPS tracking live stream).
- **Data Format:** JSON

---

## 📡 System Architecture

1. **Hardware Devices** collect data (Location, Weight, RFID).
2. Devices send data via **HTTP POST** to the Backend API.
3. **Backend** saves data to PostgreSQL.
4. **Frontend App** polls the backend or receives Push Notifications to update the UI.

---

## 1. Smart Pet Collar (GPS & Activity) 📍

**Function:** Tracks real-time location and activity levels.

### Logic Flow
1. **Wake Up:** Device wakes up on motion (accelerometer) or timer interval.
2. **Acquire GPS:** Reads Latitude/Longitude from GPS module.
3. **Check Geofence (Optional):** Checks if coordinates are outside the "Safe Zone" stored in memory.
4. **Transmit:** Sends data to Backend.
5. **Sleep:** Enters deep sleep to conserve battery.

### Data Payload (Hardware -> Backend)
**Endpoint:** `POST /api/hardware/collar/telemetry`
```json
{
  "deviceId": "COLLAR_12345",
  "petId": 5,
  "batteryLevel": 85,
  "gps": {
    "lat": 14.5995,
    "lng": 120.9842
  },
  "status": "active" // or "idle"
}
