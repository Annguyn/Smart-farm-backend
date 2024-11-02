import io
import logging
import time
from datetime import datetime, timedelta
from io import BytesIO

import PIL
from apscheduler.schedulers.background import BackgroundScheduler
from flask import Flask, request, jsonify, Response, send_file
import tensorflow as tf
import numpy as np
from PIL import Image
import requests
import cv2
from flask_cors import CORS
import sqlite3


app = Flask(__name__)

model = tf.keras.models.load_model('static/model/guava_model.keras')
class_names = ['dot', 'healthy', 'mummification', 'rust']
conn = sqlite3.connect('database.db')
cursor = conn.cursor()
cursor.execute('''CREATE TABLE IF NOT EXISTS sensor_data (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    timestamp TEXT,
                    humidity REAL,
                    temperature REAL,
                    soil_moisture INTEGER,
                    distance INTEGER,
                    pump_status TEXT)''')
conn.commit()
conn.close()
ESP32_CAM_URL = "http://192.168.1.5"
ESP8266_IP = 'http://192.168.1.8:80'

# @app.route('/predict', methods=['POST'])
# def predict():
#     if 'file' not in request.files:
#         return jsonify({'error': 'No file part'}), 400
#
#     file = request.files['file']
#     if file.filename == '':
#         return jsonify({'error': 'No selected file'}), 400
#
#     img = Image.open(file.stream).convert('RGB')
#     img = img.resize((224, 224))
#
#     img_array = tf.keras.utils.img_to_array(img)
#     img_array = np.expand_dims(img_array, axis=0)
#
#     predictions = model.predict(img_array)
#     predicted_class = class_names[np.argmax(predictions[0])]
#     confidence = round(100 * (np.max(predictions[0])), 2)
#
#     return jsonify({
#         'filename': file.filename,
#         'predicted_class': predicted_class,
#         'confidence': confidence
#     })


@app.route('/predict', methods=['POST'])
def predict():
    try:
        img_resp = requests.get("http://192.168.1.7:5000/capture", timeout=5)
        img_resp.raise_for_status()

        img = Image.open(BytesIO(img_resp.content)).convert('RGB')
        img = img.resize((224, 224))

        img_array = tf.keras.utils.img_to_array(img)
        img_array = np.expand_dims(img_array, axis=0)

        predictions = model.predict(img_array)
        predicted_class = class_names[np.argmax(predictions[0])]
        confidence = round(100 * (np.max(predictions[0])), 2)

        return jsonify({
            'predicted_class': predicted_class,
            'confidence': confidence
        })
    except requests.exceptions.RequestException as e:
        return jsonify({'error': str(e)}), 500
    except PIL.UnidentifiedImageError:
        return jsonify({'error': 'Failed to decode image from camera'}), 500
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/status', methods=['GET'])
def status():
    response = requests.get(f"{ESP8266_IP}/status")
    return response.json()

@app.route('/control', methods=['POST'])
def control():
    global pump_status
    data = request.json

    if data is None or 'action' not in data:
        return jsonify({'error': 'Invalid request'}), 400

    print("Received data:", data)
    action = data['action']

    if action == 'on':
        pump_status = 'on'
        print("Sending 'on' command to ESP8266")
        response = requests.post(f"{ESP8266_IP}/control", data={'action': 'on'})
        print("ESP8266 Response:", response.text)
    elif action == 'off':
        pump_status = 'off'
        print("Sending 'off' command to ESP8266")
        response = requests.post(f"{ESP8266_IP}/control", data={'action': 'off'})
        print("ESP8266 Response:", response.text)
    else:
        return jsonify({'error': 'Invalid action'}), 400

    return jsonify({'pump_status': pump_status})
@app.route('/mode', methods=['GET', 'POST'])
def mode_handler():
    global mode
    if request.method == 'POST':
        data = request.json
        if data is None or 'mode' not in data:
            return jsonify({'error': 'Invalid request'}), 400

        mode = data['mode']
        response = requests.post(f"{ESP8266_IP}/mode", data={'mode': mode})
        if response.status_code != 200:
            return jsonify({'error': 'Failed to update mode'}), 500

    return jsonify({'mode': mode})
@app.route('/sensor', methods=['GET'])
def sensor():
    response = requests.get(f"{ESP8266_IP}/sensor")
    return response.json()

@app.route('/capture', methods=['GET'])
def capture_handler():
    try:
        img_resp = requests.get(f"{ESP32_CAM_URL}/capture", timeout=5)
        img_resp.raise_for_status()

        img_arr = np.array(bytearray(img_resp.content), dtype=np.uint8)
        frame = cv2.imdecode(img_arr, cv2.IMREAD_COLOR)

        if frame is None or frame.size == 0:
            return jsonify({'error': 'Failed to decode image from camera'}), 500

        _, buffer = cv2.imencode('.jpg', frame)
        img_io = io.BytesIO(buffer)
        img_io.seek(0)

        return send_file(img_io, mimetype='image/jpeg')
    except requests.exceptions.RequestException as e:
        return jsonify({'error': str(e)}), 500
    except cv2.error as e:
        return jsonify({'error': 'OpenCV error: ' + str(e)}), 500

import time
@app.route('/stream')
def stream_handler():
    return Response(stream_video(), mimetype='multipart/x-mixed-replace; boundary=frame')

def stream_video():
    while True:
        try:
            img_resp = requests.get(f"{ESP32_CAM_URL}/capture", timeout=5)
            img_resp.raise_for_status()

            img_arr = np.array(bytearray(img_resp.content), dtype=np.uint8)
            frame = cv2.imdecode(img_arr, cv2.IMREAD_COLOR)

            _, buffer = cv2.imencode('.jpg', frame)
            frame = buffer.tobytes()

            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
        except requests.exceptions.RequestException as e:
            print(f"Error: {e}")
            time.sleep(1)
@app.route('/statistics', methods=['GET'])
def statistics():
    conn = sqlite3.connect('database.db')
    cursor = conn.cursor()

    filter_type = request.args.get('filter_type', 'day')
    filter_value = request.args.get('filter_value', datetime.now().strftime('%Y-%m-%d'))

    try:
        if filter_type == 'day':
            # Get start and end for the whole day
            start_value = datetime.strptime(filter_value, '%Y-%m-%d')
            end_value = start_value + timedelta(days=1)  # The end is the next day
            query = "SELECT * FROM sensor_data WHERE timestamp >= ? AND timestamp < ?"
            parameters = (start_value.strftime('%Y-%m-%d %H:%M:%S'), end_value.strftime('%Y-%m-%d %H:%M:%S'))
        elif filter_type == 'hour':
            # Get start and end for the specified hour
            start_value = datetime.strptime(filter_value.split('.')[0], '%Y-%m-%dT%H:%M:%S')
            end_value = start_value + timedelta(hours=1)
            query = "SELECT * FROM sensor_data WHERE timestamp >= ? AND timestamp < ?"
            parameters = (start_value.strftime('%Y-%m-%d %H:%M:%S'), end_value.strftime('%Y-%m-%d %H:%M:%S'))
        elif filter_type == 'month':
            # Get start and end for the whole month
            start_value = datetime.strptime(filter_value, '%Y-%m-%d')
            end_value = (start_value + timedelta(days=32)).replace(day=1)  # Move to the next month
            query = "SELECT * FROM sensor_data WHERE timestamp >= ? AND timestamp < ?"
            parameters = (start_value.strftime('%Y-%m-01 00:00:00'), end_value.strftime('%Y-%m-01 00:00:00'))
        else:
            return jsonify({'error': 'Invalid filter type'}), 400
    except ValueError as e:
        return jsonify({'error': f'Invalid date format: {str(e)}'}), 400

    cursor.execute(query, parameters)
    data = cursor.fetchall()

    def convert_values(row):
        return (
            row[0],  # id
            row[1],  # timestamp
            float(row[2]),  # humidity
            float(row[3]),  # temperature
            int(row[4]),  # soil_moisture
            row[5],  # pump_status
            int(row[6])  # distance
        )

    data = [convert_values(row) for row in data]

    conn.close()
    return jsonify(data)
@app.route('/history', methods=['GET'])
def history():
    conn = sqlite3.connect('database.db')
    cursor = conn.cursor()
    cursor.execute('''SELECT * FROM sensor_data ORDER BY timestamp DESC ''')
    data = cursor.fetchall()
    conn.close()

    return jsonify(data)
def fetch_and_store_sensor_data():
    logging.info("Fetching sensor data...")
    try:
        data = requests.get(f"{ESP8266_IP}/sensor")
        data = data.json()
        humidity = data.get('humidity')
        temperature = data.get('temperature')
        soil_moisture = data.get('soil_moisture')
        distance = data.get('distance')
        pump_status = data.get('pump_status')
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

        if None not in (humidity, temperature, soil_moisture, distance, pump_status):
            conn = sqlite3.connect('database.db')
            cursor = conn.cursor()
            cursor.execute('''INSERT INTO sensor_data (timestamp, humidity, temperature, soil_moisture, distance, pump_status)
                              VALUES (?, ?, ?, ?, ?, ?)''', (timestamp, humidity, temperature, soil_moisture, distance, pump_status))
            conn.commit()
            conn.close()
            logging.info("Sensor data stored successfully.")
        else:
            logging.warning("Received None value in sensor data.")
    except Exception as e:
        logging.error(f"Error fetching sensor data: {str(e)}")

@app.route('/sensor_data', methods=['POST'])
def sensor_data():
    fetch_and_store_sensor_data()
    return jsonify({'status': 'success'})


scheduler = BackgroundScheduler()
scheduler.add_job(fetch_and_store_sensor_data, 'interval', minutes=15)
scheduler.start()
logging.info("Scheduler started.")
if __name__ == '__main__':

    app.run(host='0.0.0.0', port=5000, debug=True)