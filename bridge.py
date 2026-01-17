import json
import os
import paho.mqtt.client as mqtt
from pymongo import MongoClient
from datetime import datetime
from dotenv import load_dotenv

# --- SECRET MANAGEMENT SETUP ---
# Load environment variables from the .env file
load_dotenv()

# Fetch secrets safely. If they are missing, default to None or raise an error.
MONGO_USER = os.getenv("MONGO_USER")
MONGO_PASS = os.getenv("MONGO_PASS")
MONGO_HOST = os.getenv("MONGO_HOST")
MONGO_PORT = os.getenv("MONGO_PORT")
MONGO_DB_NAME = os.getenv("MONGO_DB")
AUTH_SOURCE = os.getenv("MONGO_AUTH_SOURCE")

MQTT_BROKER = os.getenv("MQTT_BROKER")
MQTT_PORT = int(os.getenv("MQTT_PORT"))
MQTT_TOPIC = os.getenv("MQTT_TOPIC")
MQTT_USER=os.getenv("MQTT_USER")
MQTT_PASS=os.getenv("MQTT_PASS")

# Check if critical credentials exist before proceeding
if not MONGO_USER or not MONGO_PASS:
    raise ValueError("Error: MONGO_USER or MONGO_PASS is missing from .env file")

# --- CONFIGURATION ---
# Construct the URI dynamically using f-strings
MONGO_URI = f"mongodb://{MONGO_USER}:{MONGO_PASS}@{MONGO_HOST}:{MONGO_PORT}/{MONGO_DB_NAME}?authSource={AUTH_SOURCE}"

# --- DATABASE SETUP ---
try:
    client = MongoClient(MONGO_URI)
    db = client[MONGO_DB_NAME]
    collection = db["SensorData"]
    print(">> Connected to MongoDB (Authenticated)")
except Exception as e:
    print(f"!! Error connecting to DB: {e}")
    exit(1) # Exit strictly if DB fails

# --- MQTT CALLBACKS ---
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f">> Connected to MQTT Broker at {MQTT_BROKER}!")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"!! Failed to connect to MQTT Broker, return code {rc}")

def on_message(client, userdata, msg):
    try:
        # 1. Decode the Payload
        payload = msg.payload.decode("utf-8")
        
        # 2. Parse JSON (Handle malformed JSON errors securely)
        try:
            data = json.loads(payload)
        except json.JSONDecodeError:
            print(f"!! SECURITY ALERT: Received non-JSON payload from {msg.topic}. Ignored.")
            return # Stop processing immediately to prevent crash

        # --- SECURITY CHECK: INPUT VALIDATION ---
        # Define the exact keys your ESP32 is programmed to send
        required_keys = ["device_id", "depth", "rain", "blockage", "status"]
        
        # Check 1: Ensure ALL required keys are present
        # If a hacker sends {"virus": "start"} or partial data, this block catches it.
        if not all(key in data for key in required_keys):
            print(f"!! SECURITY ALERT: Schema Mismatch! Received keys: {list(data.keys())}")
            return # Reject the message (Do not save to DB)

        # Add Timestamp
        data["timestamp"] = datetime.utcnow()

        # Insert
        collection.insert_one(data)
        print(f"[SAVED] ID: {data['device_id']} | Depth: {data.get('depth', 'N/A')}cm | Rain: {data.get('rain', 'N/A')} | Blockage: {data.get('blockage', 'N/A')} | Status: {data.get('status', 'N/A')}")

    except Exception as e:
        print(f"Error processing message: {e}")

# --- MAIN LOOP ---
mqtt_client = mqtt.Client()
if MQTT_USER and MQTT_PASS:
    mqtt_client.username_pw_set(MQTT_USER, MQTT_PASS)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

try:
    print(f"Connecting to MQTT Broker...")
    mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
    mqtt_client.loop_forever()
except Exception as e:
    print(f"!! Could not connect to MQTT Broker: {e}")