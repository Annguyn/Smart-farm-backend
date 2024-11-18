import sqlite3

def setup_database():
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