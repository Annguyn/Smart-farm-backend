from flask import Blueprint, request, jsonify ,current_app
from dotenv import load_dotenv
import os
import requests

fan_api = Blueprint('fan_api', __name__)
load_dotenv()
ESP32_IP = os.getenv('ESP32_IP')
fanStatus = False

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
        response = requests.post(url, data=payload, headers=headers, timeout=2)
        response.raise_for_status()
        fanStatus = speed
        return jsonify({
            "message": f"Fan speed set to {speed}"
        }), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500