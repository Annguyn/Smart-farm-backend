from flask import Blueprint, request, jsonify, current_app
from dotenv import load_dotenv
import os
import requests

pump_api = Blueprint('pump_api', __name__)
load_dotenv()
ESP32_IP = os.getenv('ESP32_IP')
pumpStatus = False

@pump_api.route('/pump/on', methods=['POST'])
def turn_on_pump():
    global pumpStatus
    try:
        url = f"http://{ESP32_IP}/pump"
        payload = {'status': 'on'}
        headers = {'Content-Type': 'application/x-www-form-urlencoded'}
        response = requests.post(url, data=payload, headers=headers)
        response.raise_for_status()
        return jsonify({"message": "Pump turned on"}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@pump_api.route('/pump/off', methods=['POST'])
def turn_off_pump():
    global pumpStatus
    try:
        url = f"http://{ESP32_IP}/pump"
        payload = {'status': 'off'}
        headers = {'Content-Type': 'application/x-www-form-urlencoded'}
        response = requests.post(url, data=payload, headers=headers)
        response.raise_for_status()
        return jsonify({"message": "Pump turned off"}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500