from flask import Blueprint, request, jsonify
import os
import requests
from dotenv import load_dotenv

threshold_api = Blueprint('threshold_api', __name__)
load_dotenv()
ESP32_IP = os.getenv('ESP32_IP')

@threshold_api.route('/setThreshold', methods=['POST'])
def set_threshold():
    if 'type' in request.json and 'value' in request.json:
        threshold_type = request.json['type']
        value = request.json['value']

        if threshold_type not in ["soilMoisture", "temperatureHigh", "temperatureLow", "waterLevel", "light"]:
            return jsonify({"status": "error", "message": "Invalid threshold type"}), 400

        url = f"http://{ESP32_IP}/setThreshold"
        payload = {'type': threshold_type, 'value': value}
        response = requests.post(url, json=payload)

        if response.status_code == 200:
            return jsonify({"status": "success", f"{threshold_type}Threshold": value}), 200
        else:
            return jsonify({"status": "error", "message": response.text}), response.status_code
    else:
        return jsonify({"status": "error", "message": "Missing 'type' or 'value' parameter"}), 400