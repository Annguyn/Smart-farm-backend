import logging
from flask import Flask
from apscheduler.schedulers.background import BackgroundScheduler
from dotenv import load_dotenv
import os
from zeroconf import ServiceInfo, Zeroconf
import socket
import atexit

from database_setup import setup_database
from sensor_data_fetcher import fetch_and_store_sensor_data
from blueprint_setup import register_blueprints

def register_mdns_service(service_name, service_type, port):
    zeroconf = Zeroconf()
    ip_address = socket.inet_aton(socket.gethostbyname(socket.gethostname()))
    service_info = ServiceInfo(
        service_type,
        f"{service_name}.{service_type}",
        addresses=[ip_address],
        port=port,
        properties={},
        server=f"{service_name}.local."
    )
    zeroconf.register_service(service_info)
    return zeroconf

app = Flask(__name__)

setup_database()
load_dotenv()

ESP32_CAM_URL = os.getenv('ESP32_CAM_URL')
ESP32_IP = os.getenv('ESP32_IP')
speaker_status = False

register_blueprints(app)

scheduler = BackgroundScheduler()
scheduler.add_job(fetch_and_store_sensor_data, 'interval', seconds=60)
scheduler.start()
logging.info("Scheduler started.")

zeroconf = register_mdns_service("anfarm", "_http._tcp.local.", 5000)

if __name__ == '__main__':
    atexit.register(zeroconf.unregister_all_services)
    app.run(host='0.0.0.0', port=5000, debug=True)