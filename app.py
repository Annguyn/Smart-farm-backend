from flask import Flask, request, jsonify
import tensorflow as tf
import numpy as np
import matplotlib.pyplot as plt
import os
from PIL import Image
import io

app = Flask(__name__)

model = tf.keras.models.load_model('static/model/guava_model.keras')

class_names = ['dot', 'healthy', 'mummification', 'rust']

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

if __name__ == '__main__':
    app.run(debug=True)
