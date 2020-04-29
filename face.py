import io
import os
import picamera
#from edgetpu.detection.engine import DetectionEngine
import cv2
import numpy
import time
#from gpiozero import MotionSensor
import RPi.GPIO as GPIO

from twilio.rest import Client
from b2blaze import B2

account_sid = os.environ.get('account_sid')
auth_token  = os.environ.get('auth_token')
b2_id       = os.environ.get('b2_id')
b2_app_key  = os.environ.get('b2_app_key')
root        = os.path.dirname(os.path.realpath(__file__))
face_model  = os.path.join(root, 'haarcascade_frontalface_default.xml')

print("Face model path: " + face_model)
last_face_detected = 0

GPIO.setmode(GPIO.BCM)

GPIO.setup(23, GPIO.IN) #PIR

def alert_face_detection(faces, image):
    for (x,y,w,h) in faces:
        cv2.rectangle(image, (x,y), (x+w,y+h), (255,255,0),2)

    cv2.imwrite('result.jpg', image)
    b2 = B2(key_id=b2_id, application_key=b2_app_key)
    bucket = b2.buckets.get('monitors')

    image_file = open('result.jpg', 'rb')
    new_file = bucket.files.upload(contents=image_file, file_name='capture/result.jpg')
    print(new_file.url)
    image_file.close()

    client = Client(account_sid, auth_token)
    message = client.messages.create(to = '+14109806647',
                                     from_= '+15014564510',
                                     media_url=[new_file.url],
                                     body =  "I detected a face in the basement")


def check_for_face():
    global last_face_detected 

    time_of_motion = time.time()
    time_since_last_motion_with_detection = time_of_motion - last_face_detected

    # if we detected a face within the last 60 seconds and we're seeing motion again skip detection until later
    if time_since_last_motion_with_detection < 60:
        return

    print("Checking for faces...")

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

        print("Found "+str(len(faces))+" faces(s)")

        if time_since_last_detection > 60:
            last_face_detected = time_of_detection

            print("It's been over 1 minute since the last detection alert")

            alert_face_detection(faces, image)


            #b2 = B2(key_id=b2_id, application_key=b2_app_key)
            #bucket = b2.buckets.get('monitors')

            #image_file = open('result.jpg', 'rb')
            #new_file = bucket.files.upload(contents=image_file, file_name='capture/result.jpg')
            #print(new_file.url)

            #client = Client(account_sid, auth_token)

            #message = client.messages.create(to = '+14109806647',
            #                                 from_= '+15014564510',
            #                                 media_url=[new_file.url],
            #                                 body =  "I detected a face in the basement")
            #print(message.sid)

    return len(faces)

try:
  while True:
    time.sleep(2) # avoid pegging the cpu
    print("waiting for motion to wake me up")
    if GPIO.input(23):
      #pir.wait_for_motion()
      print("we detected motion!")

      check_for_face()


except:
    GPIO.cleanup()
