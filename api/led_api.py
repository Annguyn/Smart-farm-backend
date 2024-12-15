import os
from dotenv import load_dotenv
from flask import Blueprint, request, jsonify
import requests

led_api = Blueprint('led_api', __name__)
load_dotenv()
ESP32_IP = os.getenv('ESP32_IP')

@led_api.route('/led/on', methods=['POST'])
def control_led_on():
    try:
        url = f"http://{ESP32_IP}/led"
        payload = {'status': 'on'}
        headers = {'Content-Type': 'application/x-www-form-urlencoded'}
        response = requests.post(url, data=payload, headers=headers, timeout=2)
        response.raise_for_status()
        return jsonify(response.json()), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@led_api.route('/led/off', methods=['POST'])
def control_led_off():
    try:
        url = f"http://{ESP32_IP}/led"
        payload = {'status': 'off'}
        headers = {'Content-Type': 'application/x-www-form-urlencoded'}
        response = requests.post(url, data=payload, headers=headers, timeout=2)
        response.raise_for_status()
        return jsonify(response.json()), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500