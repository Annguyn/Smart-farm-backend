from flask import Blueprint, request, jsonify, current_app
from dotenv import load_dotenv
import os
import requests

curtain_api = Blueprint('curtain_api', __name__)
load_dotenv()
ESP32_IP = os.getenv('ESP32_IP')
curtainStatus = False

@curtain_api.route('/curtain/open', methods=['POST'])
def open_curtain():
    global curtainStatus
    try:
        url = f"http://{ESP32_IP}/curtain"
        payload = {'action': 'open'}
        headers = {'Content-Type': 'application/x-www-form-urlencoded'}
        response = requests.post(url, data=payload, headers=headers, timeout=2)
        response.raise_for_status()
        curtainStatus = True
        return jsonify({"message": "Curtain is opening"}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@curtain_api.route('/curtain/close', methods=['POST'])
def close_curtain():
    global curtainStatus
    try:
        url = f"http://{ESP32_IP}/curtain"
        payload = {'action': 'close'}
        headers = {'Content-Type': 'application/x-www-form-urlencoded'}
        response = requests.post(url, data=payload, headers=headers, timeout=2)
        response.raise_for_status()
        curtainStatus = False
        return jsonify({"message": "Curtain is closing"}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500