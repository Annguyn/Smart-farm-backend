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
cursor.execute('''CREATE TABLE IF NOT EXISTS sensor_status (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        timestamp TEXT,
                        humidity REAL,
                        temperature REAL,
                        soil_moisture INTEGER,
                        distance INTEGER,
                        pump_status TEXT,
                        light INTEGER,
                        rain_status INTEGER,
                        sound_status INTEGER,
                        motor_status INTEGER)''')
conn.commit()
conn.close()
ESP32_CAM_URL = "http://172.20.10.2"
ESP32_IP = 'http://172.20.10.2'
speaker_status = False

@app.route('/predict_file', methods=['POST'])
def predict_file():
    if 'file' not in request.files:
        return jsonify({'error': 'No file part'}), 400

    file = request.files['file']
    if file.filename == '':
        return jsonify({'error': 'No selected file'}), 400

    img = Image.open(file.stream).convert('RGB')
    img = img.resize((224, 224))

    img_array = tf.keras.utils.img_to_array(img)
    img_array = np.expand_dims(img_array, axis=0)

    predictions = model.predict(img_array)
    predicted_class = class_names[np.argmax(predictions[0])]
    confidence = round(100 * (np.max(predictions[0])), 2)

    return jsonify({
        'filename': file.filename,
        'predicted_class': predicted_class,
        'confidence': confidence
    })


@app.route('/predict', methods=['POST'])
def predict():
    try:
        img_resp = requests.get(f"{ESP32_CAM_URL}/capture", timeout=5)
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
@app.route('/motor/on', methods=['GET'])
def motor_on():
    try:
        response = requests.get(f"{ESP32_IP}/motor/on")
        response.raise_for_status()
        return jsonify({'status': 'Motor is ON'}), 200
    except requests.exceptions.RequestException as e:
        return jsonify({'error': str(e)}), 500

@app.route('/motor/off', methods=['GET'])
def motor_off():
    try:
        response = requests.get(f"{ESP32_IP}/motor/off")
        response.raise_for_status()
        return jsonify({'status': 'Motor is OFF'}), 200
    except requests.exceptions.RequestException as e:
        return jsonify({'error': str(e)}), 500

@app.route('/servo', methods=['GET'])
def servo():
    angle = request.args.get('angle')
    if not angle:
        return jsonify({'error': 'Angle parameter is required'}), 400
    try:
        response = requests.get(f"{ESP32_IP}/servo?angle={angle}")
        response.raise_for_status()
        return jsonify({'status': f'Servo moved to angle: {angle}'}), 200
    except requests.exceptions.RequestException as e:
        return jsonify({'error': str(e)}), 500

@app.route('/pump/on', methods=['GET'])
def pump_on():
    try:
        response = requests.get(f"{ESP32_IP}/pump/on")
        response.raise_for_status()
        return jsonify({'status': 'Water Pump is ON'}), 200
    except requests.exceptions.RequestException as e:
        return jsonify({'error': str(e)}), 500

@app.route('/pump/off', methods=['GET'])
def pump_off():
    try:
        response = requests.get(f"{ESP32_IP}/pump/off")
        response.raise_for_status()
        return jsonify({'status': 'Water Pump is OFF'}), 200
    except requests.exceptions.RequestException as e:
        return jsonify({'error': str(e)}), 500

@app.route('/data', methods=['GET'])
def get_data():
    try:
        response = requests.get(f"{ESP32_IP}/data")
        response.raise_for_status()
        return jsonify(response.json()), 200
    except requests.exceptions.RequestException as e:
        return jsonify({'error': str(e)}), 500
@app.route('/speaker/on', methods=['GET'])
def speaker_on():
    global speaker_status
    speaker_status = True
    return jsonify({'status': 'Speaker is ON'}), 200

@app.route('/speaker/off', methods=['GET'])
def speaker_off():
    global speaker_status
    speaker_status = False
    return jsonify({'status': 'Speaker is OFF'}), 200
@app.route('/statistics', methods=['GET'])
def statistics():
    conn = sqlite3.connect('database.db')
    cursor = conn.cursor()

    filter_type = request.args.get('filter_type', 'day')
    filter_value = request.args.get('filter_value', datetime.now().strftime('%Y-%m-%d'))

    try:
        if filter_type == 'day':
            start_value = datetime.strptime(filter_value, '%Y-%m-%d')
            end_value = start_value + timedelta(days=1)
            query = "SELECT * FROM sensor_data WHERE timestamp >= ? AND timestamp < ?"
            parameters = (start_value.strftime('%Y-%m-%d %H:%M:%S'), end_value.strftime('%Y-%m-%d %H:%M:%S'))
        elif filter_type == 'hour':
            start_value = datetime.strptime(filter_value.split('.')[0], '%Y-%m-%dT%H:%M:%S')
            end_value = start_value + timedelta(hours=1)
            query = "SELECT * FROM sensor_data WHERE timestamp >= ? AND timestamp < ?"
            parameters = (start_value.strftime('%Y-%m-%d %H:%M:%S'), end_value.strftime('%Y-%m-%d %H:%M:%S'))
        elif filter_type == 'month':
            start_value = datetime.strptime(filter_value, '%Y-%m-%d')
            end_value = (start_value + timedelta(days=32)).replace(day=1)
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
            int(row[6]),  # distance
            int(row[7]),  # light
            int(row[8]),  # rain_status
            int(row[9]),  # sound_status
            int(row[10])  # motor_status
        )

    data = [convert_values(row) for row in data]

    conn.close()
    return jsonify(data)

def fetch_and_store_sensor_data():
    try:
        response = requests.get(f"{ESP32_IP}/data")
        response.raise_for_status()
        data = response.json()

        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        humidity = data.get('humidity')
        temperature = data.get('temperature')
        soil_moisture = data.get('soilMoisture')
        distance = data.get('distance')
        pump_status = data.get('pumpStatus')
        light = data.get('light')
        rain_status = data.get('rainStatus')
        sound_status = data.get('soundStatus')
        motor_status = data.get('motorStatus')

        conn = sqlite3.connect('database.db')
        cursor = conn.cursor()
        cursor.execute('''INSERT INTO sensor_data (timestamp, humidity, temperature, soil_moisture, distance, pump_status, light, rain_status, sound_status, motor_status)
                          VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)''',
                       (timestamp, humidity, temperature, soil_moisture, distance, pump_status, light, rain_status, sound_status, motor_status))
        conn.commit()
        conn.close()
    except requests.exceptions.RequestException as e:
        print(f"Error fetching data: {e}")
    except sqlite3.Error as e:
        print(f"Database error: {e}")

scheduler = BackgroundScheduler()
scheduler.add_job(fetch_and_store_sensor_data, 'interval', minutes=15)
scheduler.start()
logging.info("Scheduler started.")
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)