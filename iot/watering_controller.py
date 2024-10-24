def control_watering(moisture_threshold):
    current_moisture = get_current_moisture_level()
    if current_moisture < moisture_threshold:
        print(f"Watering started. Current moisture: {current_moisture}")
    else:
        print(f"No need to water. Current moisture: {current_moisture}")
