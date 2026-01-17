/*
 * sample_secrets.h
 * Store all sample sensitive credentials here.
 * This file should be added to .gitignore if using GitHub.
 */

#ifndef SECRETS_H
#define SECRETS_H

// --- 1. WiFi Credentials ---
#define SECRET_SSID "Your_WiFi_Name" 
#define SECRET_WIFI_PASS "Your_WiFi_Password"

// --- 2. GCP / MQTT Credentials ---
#define SECRET_MQTT_BROKER "YOUR_GCP_EXTERNAL_IP"
#define SECRET_MQTT_PORT 1883

// Security Measure: MQTT Authentication
// These must match the user you created on the GCP VM (mosquitto_passwd)
#define SECRET_MQTT_USER "YOUR_MQTT_USERNAME"
#define SECRET_MQTT_PASS "YOUR_SECURE_PASSWORD"

#define SECRET_MQTT_TOPIC "flowguard"
#define SECRET_DEVICE_ID "ESP32_Node_01"

#endif
