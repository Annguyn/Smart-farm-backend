import logging
from flask import Flask
from apscheduler.schedulers.background import BackgroundScheduler
from dotenv import load_dotenv
import os

from database_setup import setup_database
from sensor_data_fetcher import fetch_and_store_sensor_data
from blueprint_setup import register_blueprints

app = Flask(__name__)

setup_database()
load_dotenv()

ESP32_CAM_URL = os.getenv('ESP32_CAM_URL')
ESP32_IP = os.getenv('ESP32_IP')
speaker_status = False

register_blueprints(app)

scheduler = BackgroundScheduler()
scheduler.add_job(fetch_and_store_sensor_data, 'interval', minutes=15)
scheduler.start()
logging.info("Scheduler started.")

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)