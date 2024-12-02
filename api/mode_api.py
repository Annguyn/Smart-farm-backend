import os

from dotenv import load_dotenv
from flask import Blueprint, jsonify
import requests

mode_api = Blueprint('mode_api', __name__)
load_dotenv()
ESP32_IP = os.getenv('ESP32_IP')
automaticCurtain = False
automaticFan = False
automaticPump = False


@mode_api.route("/fan/mode/automatic", methods=["POST"])
def set_fan_automatic():
    try:
        url = f"http://{ESP32_IP}/fan/automatic/on"
        response = requests.post(url, data="")
        return jsonify({
            "message": "Fan automatic mode is ON",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@mode_api.route("/fan/mode/manual", methods=["POST"])
def set_fan_manual():
    try:
        url = f"http://{ESP32_IP}/fan/automatic/off"
        response = requests.post(url, data="")
        return jsonify({
            "message": "Fan manual mode is OFF",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@mode_api.route("/pump/mode/automatic", methods=["POST"])
def set_pump_automatic():
    try:
        url = f"http://{ESP32_IP}/pump/automatic/on"
        response = requests.post(url, data="")
        return jsonify({
            "message": "Pump automatic mode is ON",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@mode_api.route("/pump/mode/manual", methods=["POST"])
def set_pump_manual():
    try:
        url = f"http://{ESP32_IP}/pump/automatic/off"
        response = requests.post(url, data="")
        return jsonify({
            "message": "Pump manual mode is OFF",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@mode_api.route("/curtain/mode/automatic", methods=["POST"])
def set_curtain_automatic():
    try:
        url = f"http://{ESP32_IP}/curtain/automatic/on"
        response = requests.post(url, data="")
        return jsonify({
            "message": "Curtain automatic mode is ON",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@mode_api.route("/curtain/mode/manual", methods=["POST"])
def set_curtain_manual():
    try:
        url = f"http://{ESP32_IP}/curtain/automatic/off"
        response = requests.post(url, data="")
        return jsonify({
            "message": "Curtain manual mode is OFF",
            "esp_response": response.text
        }), response.status_code
    except Exception as e:
        return jsonify({"error": str(e)}), 500
