from flask import Blueprint, request, jsonify
from dotenv import load_dotenv
import os
import requests

fan_api = Blueprint('fan_api', __name__)
load_dotenv()
ESP32_IP = os.getenv('ESP32_IP')

@fan_api.route('/fan/set', methods=['POST'])
def set_fan_speed():
    try:
        speed = request.json.get('speed', 0)
        url = f"http://{ESP32_IP}/fan/set"
        payload = f"speed={speed}"
        response = requests.post(url, data=payload)
        return jsonify({
            "message": f"Fan speed set to {speed}",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500