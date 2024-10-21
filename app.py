from flask import Flask, request, jsonify
import tensorflow as tf
from tensorflow.keras.models import load_model
from PIL import Image
import numpy as np
import io
import sys
sys.stdout.reconfigure(encoding='utf-8')

app = Flask(__name__)

# Load your trained model
model = load_model('static/model/guava-w-aug-esp32.h5')

CLASS_NAMES = ['dot', 'healthy', 'mummification', 'rust']

def preprocess_image(image):
    image = image.resize((224, 224))
    image = np.array(image) / 255.0
    image = np.expand_dims(image, axis=0)
    return image

@app.route('/predict', methods=['POST'])
def predict():
    if 'file' not in request.files:
        return jsonify({'error': 'No file part'}), 400

    file = request.files['file']

    if file.filename == '':
        return jsonify({'error': 'No selected file'}), 400

    try:
        image = Image.open(io.BytesIO(file.read()))
        processed_image = preprocess_image(image)

        predictions = model.predict(processed_image)
        predicted_class = CLASS_NAMES[np.argmax(predictions)]

        return jsonify({'prediction': predicted_class}), 200

    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True)
