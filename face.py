import io
import os
import picamera
import cv2
import numpy
import time
from gpiozero import MotionSensor
from twilio.rest import Client

account_sid = os.environ.get('account_sid')
auth_token  = os.environ.get('auth_token')
root        = os.path.dirname(os.path.realpath(__file__))
face_model = os.path.join(root, 'haarcascade_frontalface_default.xml')
print "Face model path: " + face_model
last_face_detected = 0


def check_for_face():
    global last_face_detected 
    print "Checking for faces..."

    stream = io.BytesIO()

    with picamera.PiCamera() as camera:
        camera.resolution = (320, 240)
        camera.capture(stream, format='jpeg')

    buff = numpy.fromstring(stream.getvalue(), dtype=numpy.uint8)

    image = cv2.imdecode(buff, 1)

    camera.close()

    face_cascade = cv2.CascadeClassifier(face_model)

    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

    faces = face_cascade.detectMultiScale(gray, 1.1, 5)

    if len(faces) > 0:
        time_of_detection = time.time()

        time_since_last_detection = time_of_detection - last_face_detected

        print "Found "+str(len(faces))+" faces(s)"

        if time_since_last_detection > 60:
            last_face_detected = time_of_detection

            print "It's been over 1 minute since the last detection alert"

            for (x,y,w,h) in faces:
                cv2.rectangle(image, (x,y), (x+w,y+h), (255,255,0),2)

            cv2.imwrite('result.jpg', image)

            client = Client(account_sid, auth_token)

            message = client.messages.create(to = '+14109806647',
                                             from_= '+15014564510',
                                             body =  "I detected a face in the basement")

    return len(faces)

# we have pin 4 plugged into the motion sensor on our pi
pir    = MotionSensor(4)

while True:
    #print("waiting for motion to wake me up")
    pir.wait_for_motion()
    #print("we detected motion!")

    check_for_face()