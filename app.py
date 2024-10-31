import io
import time
from io import BytesIO

from flask import Flask, request, jsonify, Response, send_file
import tensorflow as tf
import numpy as np
from PIL import Image
import requests
import cv2
from flask_cors import CORS

app = Flask(__name__)

model = tf.keras.models.load_model('static/model/guava_model.keras')
class_names = ['dot', 'healthy', 'mummification', 'rust']
ESP32_CAM_URL = "http://192.168.1.5"
ESP8266_IP = 'http://192.168.1.5:80'

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
    img_resp = requests.get(ESP32_CAM_URL)
    if img_resp.status_code != 200:
        return jsonify({'error': 'Failed to capture image from camera'}), 500

    img = Image.open(BytesIO(img_resp.content)).convert('RGB')
    img = img.resize((224, 224))

    # Preprocess and predict
    img_array = tf.keras.utils.img_to_array(img)
    img_array = np.expand_dims(img_array, axis=0)

    predictions = model.predict(img_array)
    predicted_class = class_names[np.argmax(predictions[0])]
    confidence = round(100 * (np.max(predictions[0])), 2)

    return jsonify({
        'predicted_class': predicted_class,
        'confidence': confidence
    })

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
    img_resp = requests.get(ESP32_CAM_URL)
    img_arr = np.array(bytearray(img_resp.content), dtype=np.uint8)
    frame = cv2.imdecode(img_arr, -1)

    _, buffer = cv2.imencode('.jpg', frame)
    img_io = io.BytesIO(buffer)
    img_io.seek(0)

    return send_file(img_io, mimetype='image/jpeg')

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




if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)