def get_sensor_data():
    temperature = get_temperature()
    humidity = get_humidity()
    soil_moisture = get_soil_moisture()
    return {
        'temperature': temperature,
        'humidity': humidity,
        'soil_moisture': soil_moisture
    }
