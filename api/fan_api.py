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
        if request.is_json:
            speed = request.json.get('speed')
        else:
            speed = request.form.get('speed') or request.args.get('speed')
        if speed is None or speed == "":
            return jsonify({"error": "Missing 'speed' parameter"}), 400
        url = f"http://{ESP32_IP}/fan/set"
        payload = {'speed': speed}
        headers = {'Content-Type': 'application/x-www-form-urlencoded'}
        response = requests.post(url, data=payload, headers=headers)
        return jsonify({
            "message": f"Fan speed set to {speed}",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500