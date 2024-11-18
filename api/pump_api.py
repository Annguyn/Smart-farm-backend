from flask import Blueprint, request, jsonify
from dotenv import load_dotenv
import os
import requests

pump_api = Blueprint('pump_api', __name__)
load_dotenv()
ESP32_IP = os.getenv('ESP32_IP')

@pump_api.route('/pump/on', methods=['POST'])
def turn_on_pump():
    try:
        url = f"http://{ESP32_IP}/pump/on"
        response = requests.post(url, data="")
        return jsonify({
            "message": "Pump is ON",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@pump_api.route('/pump/off', methods=['POST'])
def turn_off_pump():
    try:
        url = f"http://{ESP32_IP}/pump/off"
        response = requests.post(url, data="")
        return jsonify({
            "message": "Pump is OFF",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500