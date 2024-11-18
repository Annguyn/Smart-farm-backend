from flask import Blueprint, request, jsonify

mode_api = Blueprint('mode_api', __name__)

automaticCurtain = False
automaticFan = False
automaticPump = False

@mode_api.route('/curtain/mode/automatic', methods=['POST'])
def set_curtain_automatic():
    global automaticCurtain
    automaticCurtain = True
    return jsonify({'message': 'Curtain is in automatic mode'}), 200

@mode_api.route('/curtain/mode/manual', methods=['POST'])
def set_curtain_manual():
    global automaticCurtain
    automaticCurtain = False
    return jsonify({'message': 'Curtain is in manual mode'}), 200

@mode_api.route('/motor/mode/automatic', methods=['POST'])
def set_motor_automatic():
    global automaticFan
    automaticFan = True
    return jsonify({'message': 'Fan is in automatic mode'}), 200

@mode_api.route('/motor/mode/manual', methods=['POST'])
def set_motor_manual():
    global automaticFan
    automaticFan = False
    return jsonify({'message': 'Fan is in manual mode'}), 200

@mode_api.route('/pump/mode/automatic', methods=['POST'])
def set_pump_automatic():
    global automaticPump
    automaticPump = True
    return jsonify({'message': 'Pump is in automatic mode'}), 200

@mode_api.route('/pump/mode/manual', methods=['POST'])
def set_pump_manual():
    global automaticPump
    automaticPump = False
    return jsonify({'message': 'Pump is in manual mode'}), 200