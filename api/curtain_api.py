from flask import Blueprint, request, jsonify
from dotenv import load_dotenv
import os
import requests

curtain_api = Blueprint('curtain_api', __name__)
load_dotenv()
ESP32_IP = os.getenv('ESP32_IP')

@curtain_api.route('/curtain/open', methods=['POST'])
def open_curtain():
    try:
        url = f"http://{ESP32_IP}/curtain/open"
        response = requests.post(url, data="")
        return jsonify({
            "message": "Curtain is opening",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@curtain_api.route('/curtain/close', methods=['POST'])
def close_curtain():
    try:
        url = f"http://{ESP32_IP}/curtain/close"
        response = requests.post(url, data="")
        return jsonify({
            "message": "Curtain is closing",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500