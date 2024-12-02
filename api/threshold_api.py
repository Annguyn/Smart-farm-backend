from flask import Blueprint, request, jsonify
import os
import requests
from dotenv import load_dotenv

threshold_api = Blueprint('threshold_api', __name__)
load_dotenv()
ESP32_IP = os.getenv('ESP32_IP')

thresholds = {
    'soil_moisture': 2000,
    'temperature_high': 35,
    'temperature_low': 20,
    'water_level': 2000
}


def send_to_esp32(endpoint, value):
    url = f"http://{ESP32_IP}/{endpoint}"
    response = requests.post(url, json={'value': value})
    return response.status_code, response.text


@threshold_api.route('/threshold/soilMoisture', methods=['POST'])
def set_soil_moisture_threshold():
    if 'value' in request.json:
        thresholds['soil_moisture'] = request.json['value']
        status_code, response_text = send_to_esp32('threshold/soilMoisture', thresholds['soil_moisture'])
        return jsonify(message=f"Soil moisture threshold set to {thresholds['soil_moisture']}",
                       esp32_response=response_text), status_code
    else:
        return jsonify(error="Missing 'value' parameter"), 400


@threshold_api.route('/threshold/temperatureHigh', methods=['POST'])
def set_temperature_high_threshold():
    if 'value' in request.json:
        thresholds['temperature_high'] = request.json['value']
        status_code, response_text = send_to_esp32('threshold/temperatureHigh', thresholds['temperature_high'])
        return jsonify(message=f"High temperature threshold set to {thresholds['temperature_high']}",
                       esp32_response=response_text), status_code
    else:
        return jsonify(error="Missing 'value' parameter"), 400


@threshold_api.route('/threshold/temperatureLow', methods=['POST'])
def set_temperature_low_threshold():
    if 'value' in request.json:
        thresholds['temperature_low'] = request.json['value']
        status_code, response_text = send_to_esp32('threshold/temperatureLow', thresholds['temperature_low'])
        return jsonify(message=f"Low temperature threshold set to {thresholds['temperature_low']}",
                       esp32_response=response_text), status_code
    else:
        return jsonify(error="Missing 'value' parameter"), 400


@threshold_api.route('/threshold/waterLevel', methods=['POST'])
def set_water_level_threshold():
    if 'value' in request.json:
        thresholds['water_level'] = request.json['value']
        status_code, response_text = send_to_esp32('threshold/waterLevel', thresholds['water_level'])
        return jsonify(message=f"Water level threshold set to {thresholds['water_level']}",
                       esp32_response=response_text), status_code
    else:
        return jsonify(error="Missing 'value' parameter"), 400
