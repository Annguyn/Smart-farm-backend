from flask import Flask, request, jsonify
import tensorflow as tf
import numpy as np
from PIL import Image
import requests

app = Flask(__name__)

model = tf.keras.models.load_model('static/model/guava_model.keras')
class_names = ['dot', 'healthy', 'mummification', 'rust']

ESP8266_IP = 'http://192.168.1.6:80'

@app.route('/predict', methods=['POST'])
def predict():
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

@app.route('/sensor', methods=['GET'])
def sensor():
    response = requests.get(f"{ESP8266_IP}/sensor")  
    return response.json()

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
