from flask import Blueprint, request, jsonify, Response, send_file
from dotenv import load_dotenv
import os
import requests
import cv2
import numpy as np
import io
import time
from PIL import Image
import tensorflow as tf
import PIL

model = tf.keras.models.load_model('static/model/guava_model.keras')
class_names = ['dot', 'healthy', 'rust']

esp32_cam_api = Blueprint('esp32_cam_api', __name__)
load_dotenv()
ESP32_CAM_URL = os.getenv('ESP32_CAM_URL')

@esp32_cam_api.route('/capture', methods=['GET'])
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

@esp32_cam_api.route('/stream')
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

@esp32_cam_api.route('/predict', methods=['POST'])
def predict():
    try:
        img_resp = requests.get(f"{ESP32_CAM_URL}/capture", timeout=5)
        img_resp.raise_for_status()

        img = Image.open(io.BytesIO(img_resp.content)).convert('RGB')
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

@esp32_cam_api.route('/predict_file', methods=['POST'])
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
