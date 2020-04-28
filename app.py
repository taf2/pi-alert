# based on stream-http.py but using flask
# adding controls to the web interface to only enable camera on demand instead of having it be always on
# this should* in theory save a lot of battery we can also possibly experiement with using webrtc and rstp streaming

import os
from importlib import import_module
from flask import Flask, render_template, Response
Camera = import_module('camera_pi').Camera

app = Flask(__name__)

@app.route('/')
def hello_world():
  return render_template('app.html')

def gen(camera):
  while True:
    frame = camera.get_frame()
    yield (b'--frame\r\n' b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')

@app.route('/stream.mjpg')
def video_feed():
  return Response(gen(Camera()), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/stop_camera')
def stop_camera():
  Camera().stop()
  return ''

if __name__ == '__main__':
    app.run(host='0.0.0.0', threaded=True)
