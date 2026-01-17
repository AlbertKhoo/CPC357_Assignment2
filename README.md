# CPC357_Assignment2
---
# FlowGuard: Smart Urban Drainage & Blockage Detection

**FlowGuard** is an IoT-based smart city solution designed to monitor urban drainage systems in real-time. It detects water levels and identifies physical blockages (trash/debris) using a laser tripwire system, helping municipalities prevent flash floods and optimize maintenance schedules.

## Project Overview

* **Edge Device:** ESP32 (Dual-core) with FreeRTOS
* **Sensors:** HC-SR04 (Ultrasonic), KY-008 Laser, GL5528 LDR, Rain Sensor
* **Connectivity:** WiFi + MQTT (Secure Authentication)
* **Backend:** Google Cloud Platform (Compute Engine), Mosquitto Broker, MongoDB
* **Visualization:** V-ONE Cloud Dashboard

## Repository Structure

| File / Folder | Description |
| :--- | :--- |
| `FlowGuard.ino` | Main firmware for ESP32. Handles sensor reading, logic, and MQTT publishing. |
| `sample_secrets.h` | (Template) C++ header file for managing WiFi & MQTT credentials on the ESP32. |
| `bridge.py` | Python middleware that bridges MQTT data to MongoDB and V-ONE Cloud. |
| `flowguard-bridge.service` | Systemd service file to run the bridge script as a background daemon on Linux. |
| `.sample.env` | (Template) Environment variables file for Python secrets management. |

---

## Development Environment Setup

Follow these steps to set up the project locally and on the cloud.

### 1. Hardware & Firmware (ESP32)

**Prerequisites:**
* [Arduino IDE](https://www.arduino.cc/en/software)
* **Libraries:** `PubSubClient` (by Nick O'Leary), `WiFi` (Built-in)

**Setup:**
1.  Clone this repository.
2.  Open `FlowGuard.ino` in Arduino IDE.
3.  Create a new tab named `secrets.h` and populate it with your credentials:
    ```cpp
    #ifndef SECRETS_H
    #define SECRETS_H
    #define SECRET_SSID "Your_WiFi_Name"
    #define SECRET_WIFI_PASS "Your_WiFi_Password"
    #define SECRET_MQTT_BROKER "YOUR_GCP_EXTERNAL_IP"
    #define SECRET_MQTT_PORT 1883
    #define SECRET_MQTT_USER "YOUR_MQTT_USERNAME"
    #define SECRET_MQTT_PASS "YOUR_SECURE_PASSWORD"
    #define SECRET_MQTT_TOPIC "flowguard"
    #define SECRET_DEVICE_ID "ESP32_Node_01"
    #endif
    ```
4.  Select your Board (NodeMCU-32S) and Port, then Upload.

---

### 2. Cloud Backend (GCP VM - Ubuntu 24.04)

**Prerequisites:**
* Python 3.10+
* Mosquitto MQTT Broker
* MongoDB Community Edition

**Installation:**
1.  **Install Dependencies:**
    ```bash
    sudo apt update && sudo apt install python3-pip mosquitto mongodb -y
    sudo pip3 install paho-mqtt pymongo python-dotenv --break-system-packages
    ```

2.  **Configure Environment Variables:**
    Create a `.env` file in the same directory as `bridge.py`:
    ```ini
    # MQTT Config
    MQTT_BROKER=localhost
    MQTT_PORT=1883
    MQTT_TOPIC=flowguard
    MQTT_USER=YOUR_MQTT_USERNAME
    MQTT_PASS=YOUR_SECURE_PASSWORD

    # Database Config
    MONGO_USER=YOUR_MONGODB_USERNAME
    MONGO_PASS=YOUR_MONGODB_PASSWORD
    MONGO_HOST=localhost
    MONGO_PORT=27017
    MONGO_DB=FlowGuardDB
    MONGO_AUTH_SOURCE=admin
    ```

3.  **Run the Bridge Script:**
    Test manually:
    ```bash
    python3 bridge.py
    ```

---

### 3. Service Automation (Systemd)

To run the Python bridge automatically on boot:

1.  Copy the service file to the system directory:
    ```bash
    sudo cp flowguard-bridge.service /etc/systemd/system/
    ```
2.  Edit the file to match your username and paths:
    ```bash
    sudo nano /etc/systemd/system/flowguard-bridge.service
    # Update 'User', 'WorkingDirectory', and 'ExecStart' paths
    ```
3.  Enable and Start:
    ```bash
    sudo systemctl daemon-reload
    sudo systemctl enable flowguard-bridge.service
    sudo systemctl start flowguard-bridge.service
    ```
4.  Check Status:
    ```bash
    sudo systemctl status flowguard-bridge.service
    ```

## Security Measures

* **Secrets Management:** Credentials are separated into `secrets.h` and `.env` (git-ignored).
* **Authentication:** Mosquitto Broker enforces `allow_anonymous false` with password protection.
* **Input Validation:** Python bridge strictly validates JSON schema and keys to prevent injection.
* **Authorization:** MongoDB implements Least Privilege Access to allow only write and read permissions.

---

## Contributors

* **Student 1:** [Khoo Kaa Hong] (164562)
* **Student 2:** [Axler Chin Shun Yuan] (162331)

---

## License

This project is submitted for academic assessment at Universiti Sains Malaysia. All code logic regarding the pulse modulation and blockage algorithm is original work.
