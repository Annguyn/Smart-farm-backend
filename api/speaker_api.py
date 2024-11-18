from flask import Blueprint, jsonify

speaker_api = Blueprint('speaker_api', __name__)
speaker_status = False

@speaker_api.route('/speaker/on', methods=['GET'])
def speaker_on():
    global speaker_status
    speaker_status = True
    return jsonify({'status': 'Speaker is ON'}), 200

@speaker_api.route('/speaker/off', methods=['GET'])
def speaker_off():
    global speaker_status
    speaker_status = False
    return jsonify({'status': 'Speaker is OFF'}), 200