from flask import Flask

from api.led_api import led_api
from api.fan_api import fan_api
from api.mode_api import mode_api
from api.pump_api import pump_api
from api.curtain_api import curtain_api
from api.speaker_api import speaker_api
from api.esp32_cam_api import esp32_cam_api
from api.sensor_api import sensor_api
from api.servo_api import servo_api
from api.threshold_api import threshold_api


def register_blueprints(app: Flask):
    app.register_blueprint(fan_api)
    app.register_blueprint(pump_api)
    app.register_blueprint(curtain_api)
    app.register_blueprint(speaker_api)
    app.register_blueprint(esp32_cam_api)
    app.register_blueprint(sensor_api)
    app.register_blueprint(servo_api)
    app.register_blueprint(mode_api)
    app.register_blueprint(led_api)
    app.register_blueprint(threshold_api)