from flask import Blueprint, request, jsonify
import os
import requests
from dotenv import load_dotenv
import logging

threshold_api = Blueprint('threshold_api', __name__)
load_dotenv()
ESP32_IP = os.getenv('ESP32_IP')

VALID_THRESHOLD_TYPES = ["soilMoisture", "temperatureHigh", "temperatureLow", "waterLevel", "light"]

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

@threshold_api.route('/setThreshold', methods=['POST'])
def set_threshold():
    data = request.get_json()

    if not data:
        logger.error("No data provided")
        return jsonify({"status": "error", "message": "No data provided"}), 400

    threshold_type = data.get('type')
    value = data.get('value')

    if not threshold_type or not value:
        logger.error("Missing 'type' or 'value' parameter")
        return jsonify({"status": "error", "message": "Missing 'type' or 'value' parameter"}), 400

    if threshold_type not in VALID_THRESHOLD_TYPES:
        logger.error(f"Invalid threshold type: {threshold_type}")
        return jsonify({"status": "error", "message": f"Invalid threshold type: {threshold_type}"}), 400

    esp32_url = f"http://{ESP32_IP}/setThreshold"
    payload = {'type': threshold_type, 'value': value}
    logger.info(f"Sending payload to ESP32: {payload}")

    try:
        response = requests.post(esp32_url, data=payload)
        logger.info(f"ESP32 response status: {response.status_code}")
        logger.info(f"ESP32 response text: {response.text}")
        response.raise_for_status()
        return jsonify({"status": "success", f"{threshold_type}Threshold": value}), 200
    except requests.exceptions.RequestException as e:
        logger.error(f"Failed to communicate with ESP32: {str(e)}")
        return jsonify({"status": "error", "message": f"Failed to communicate with ESP32: {str(e)}"}), 500