import requests
import sqlite3
from datetime import datetime
from dotenv import load_dotenv
import os

load_dotenv()
ESP32_IP = os.getenv('ESP32_IP')

def fetch_and_store_sensor_data():
    try:
        response = requests.get(f"{ESP32_IP}/data")
        response.raise_for_status()
        data = response.json()

        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        humidity = data.get('humidity')
        temperature = data.get('temperature')
        soil_moisture = data.get('soilMoisture')
        distance = data.get('distance')
        pump_status = data.get('pumpStatus')
        light = data.get('light')
        rain_status = data.get('rainStatus')
        sound_status = data.get('soundStatus')
        motor_status = data.get('motorStatus')
        fan_status = data.get('fanStatus')
        curtain_status = data.get('curtainStatus')
        automatic_fan = data.get('automaticFan')
        automatic_pump = data.get('automaticPump')
        automatic_curtain = data.get('automaticCurtain')

        conn = sqlite3.connect('database.db')
        cursor = conn.cursor()
        cursor.execute('''INSERT INTO sensor_data (timestamp, humidity, temperature, soil_moisture, distance, pump_status, light, rain_status, sound_status, motor_status, fan_status, curtain_status, automatic_fan, automatic_pump, automatic_curtain)
                          VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)''',
                       (timestamp, humidity, temperature, soil_moisture, distance, pump_status, light, rain_status, sound_status, motor_status, fan_status, curtain_status, automatic_fan, automatic_pump, automatic_curtain))
        conn.commit()
        conn.close()
    except requests.exceptions.RequestException as e:
        print(f"Error fetching data: {e}")
    except sqlite3.Error as e:
        print(f"Database error: {e}")