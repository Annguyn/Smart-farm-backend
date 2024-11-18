import os

from dotenv import load_dotenv
from flask import Blueprint, request, jsonify
import requests

servo_api = Blueprint('servo_api', __name__)
load_dotenv()
ESP32_IP = os.getenv("ESP32_IP")


@servo_api.route('/servo/up', methods=['POST'])
def move_servo_up():
    try:
        url = f"http://{ESP32_IP}/servo/up"
        response = requests.post(url)
        return jsonify({
            "message": "Servo moved up",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@servo_api.route('/servo/down', methods=['POST'])
def move_servo_down():
    try:
        url = f"http://{ESP32_IP}/servo/down"
        response = requests.post(url)
        return jsonify({
            "message": "Servo moved down",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@servo_api.route('/servo/left', methods=['POST'])
def move_servo_left():
    try:
        url = f"http://{ESP32_IP}/servo/left"
        response = requests.post(url)
        return jsonify({
            "message": "Servo moved left",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@servo_api.route('/servo/right', methods=['POST'])
def move_servo_right():
    try:
        url = f"http://{ESP32_IP}/servo/right"
        response = requests.post(url)
        return jsonify({
            "message": "Servo moved right",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500