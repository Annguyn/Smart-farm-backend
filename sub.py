import tkinter as tk
from tkinter import filedialog
import tensorflow as tf
import numpy as np
import matplotlib.pyplot as plt

model = tf.keras.models.load_model('static/model/guava_model.keras')

class_names = ['dot', 'healthy', 'mummification', 'rust']

def choose_file():
    root = tk.Tk()
    root.withdraw()
    file_path = filedialog.askopenfilename(title="Select an Image File",
                                           filetypes=[("Image Files", "*.png;*.jpg;*.jpeg")])
    return file_path

file_path = choose_file()

if file_path:
    img = tf.keras.utils.load_img(file_path, target_size=(224, 224))
    img_array = tf.keras.utils.img_to_array(img)
    img_array = tf.expand_dims(img_array, 0)

    predictions = model.predict(img_array)
    predicted_class = class_names[np.argmax(predictions[0])]
    confidence = round(100 * (np.max(predictions[0])), 2)

    print(f"Image: {file_path.split('/')[-1]}")
    print(f"Predicted class: {predicted_class}")
    print(f"Confidence: {confidence}%")
    plt.imshow(img)
    plt.axis('off')
    plt.show()
else:
    print("No file selected.")
