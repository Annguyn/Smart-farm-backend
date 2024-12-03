import os
from dotenv import load_dotenv
from flask import Blueprint, request, jsonify
import requests

servo_api = Blueprint('servo_api', __name__)
load_dotenv()
ESP32_IP = os.getenv("ESP32_IP")

@servo_api.route('/servo', methods=['POST'])
def move_servo():
    try:
        if request.is_json:
            direction = request.json.get('direction')
        else:
            direction = request.form.get('direction') or request.args.get('direction')
        if direction is None or direction == "":
            return jsonify({"error": "Missing 'direction' parameter"}), 400
        url = f"http://{ESP32_IP}/servo"
        payload = {'direction': direction}
        headers = {'Content-Type': 'application/x-www-form-urlencoded'}
        response = requests.post(url, data=payload, headers=headers, timeout=2)
        response.raise_for_status()
        return jsonify(response.json()), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@servo_api.route('/servo/up', methods=['POST'])
def move_servo_up():
    try:
        url = f"http://{ESP32_IP}/servo/up"
        response = requests.post(url)
        return jsonify({
            "message": "Servo moved up",
        })
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@servo_api.route('/servo/down', methods=['POST'])
def move_servo_down():
    try:
        url = f"http://{ESP32_IP}/servo/down"
        response = requests.post(url)
        return jsonify({
            "message": "Servo moved down",
        })
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@servo_api.route('/servo/left', methods=['POST'])
def move_servo_left():
    try:
        url = f"http://{ESP32_IP}/servo/left"
        response = requests.post(url)
        return jsonify({
            "message": "Servo moved left",
        })
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@servo_api.route('/servo/right', methods=['POST'])
def move_servo_right():
    try:
        url = f"http://{ESP32_IP}/servo/right"
        response = requests.post(url)
        return jsonify({
            "message": "Servo moved right",
        })
    except Exception as e:
        return jsonify({"error": str(e)}), 500