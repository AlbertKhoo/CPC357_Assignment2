/*
 * Project: Smart Urban Drainage & Blockage Detection (FlowGuard)
 * Target: SDG 11 - Sustainable Cities
 * Security Level: High (Secrets Management + MQTT Auth)
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

// --- 1. CONFIGURATION ---
const char* ssid = SECRET_SSID;
const char* password = SECRET_WIFI_PASS;
const char* mqtt_server = SECRET_MQTT_BROKER;
const int mqtt_port = SECRET_MQTT_PORT;
const char* mqtt_user = SECRET_MQTT_USER;
const char* mqtt_pass = SECRET_MQTT_PASS;

const char* mqtt_topic = SECRET_MQTT_TOPIC;
const char* device_id = SECRET_DEVICE_ID;

// --- 2. PIN DEFINITIONS ---
#define TRIG_PIN    5   
#define ECHO_PIN    18  
#define LASER_PIN   4   
#define LDR_PIN     34  
#define RAIN_PIN    19  

// --- 3. CONSTANTS ---
const int LDR_THRESHOLD = 2000;    
const int BLOCKAGE_TIME = 3000;    
const int DRAIN_TOTAL_DEPTH = 16;  

// --- 4. GLOBAL OBJECTS ---
WiFiClient espClient;
PubSubClient client(espClient);

// Logic Variables
long duration;
int distance;
int waterLevel;
int ldrValue;
unsigned long blockageStartTime = 0;
bool isBlockageDetected = false;
bool blockageConfirmed = false;
unsigned long lastMsg = 0;

// --- SETUP WIFI ---
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// --- RECONNECT TO MQTT BROKER (SECURE) ---
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection... ");
    
    // Connect using Username & Password from secrets.h
    if (client.connect(device_id, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Pin Setup
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LASER_PIN, OUTPUT);
  pinMode(RAIN_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);

  // Network Setup
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  // 1. Maintain MQTT Connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 2. Main Logic (Every 2 seconds)
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    // --- A. READ WATER LEVEL ---
    digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    duration = pulseIn(ECHO_PIN, HIGH);
    distance = duration * 0.034 / 2;
    waterLevel = DRAIN_TOTAL_DEPTH - distance;
    if (waterLevel < 0) waterLevel = 0;

    // --- B. READ RAIN SENSOR ---
    bool isRaining = (digitalRead(RAIN_PIN) == LOW);

    // --- C. BLOCKAGE DETECTION ---
    digitalWrite(LASER_PIN, HIGH); delay(10);
    ldrValue = analogRead(LDR_PIN);
    digitalWrite(LASER_PIN, LOW);

    // Logic: HIGH value = Blocked (Dark), LOW value = Clear (Bright)
    bool beamBroken = (ldrValue > LDR_THRESHOLD); 

    if (beamBroken) {
      if (!isBlockageDetected) {
        blockageStartTime = millis();
        isBlockageDetected = true;
      } else if ((millis() - blockageStartTime) > BLOCKAGE_TIME) {
        blockageConfirmed = true;
      }
    } else {
      isBlockageDetected = false;
      blockageConfirmed = false;
    }

    // --- D. DETERMINE STATUS MESSAGE ---
    String statusMsg = "Normal";
    if (waterLevel > (DRAIN_TOTAL_DEPTH - 5)) statusMsg = "Critical Flood";
    else if (blockageConfirmed && !isRaining) statusMsg = "Illegal Dumping";
    else if (blockageConfirmed && isRaining) statusMsg = "Debris Flow";
    else if (isRaining) statusMsg = "Raining";

    // --- E. PREPARE JSON PAYLOAD ---
    // Manually constructing JSON string
    String jsonPayload = "{";
    jsonPayload += "\"device_id\": \"" + String(device_id) + "\",";
    jsonPayload += "\"depth\": " + String(waterLevel) + ",";
    jsonPayload += "\"rain\": \"" + String(isRaining ? "YES" : "NO") + "\",";
    jsonPayload += "\"blockage\": \"" + String(blockageConfirmed ? "YES" : "NO") + "\",";
    jsonPayload += "\"status\": \"" + statusMsg + "\"";
    jsonPayload += "}";

    // --- F. PUBLISH TO GCP ---
    Serial.print("LDR Value: "); Serial.print(ldrValue);
    Serial.print(" | Publishing: ");
    Serial.println(jsonPayload);
    
    client.publish(mqtt_topic, jsonPayload.c_str());
  }
}