from picamera import PiCamera
from time import sleep
from gpiozero import MotionSensor

pir    = MotionSensor(4)

while True:
    print("waiting for motion to wake me up")
    pir.wait_for_motion()
    print("we detected motion!")
    camera = PiCamera()

    camera.resolution = (1024, 768)
    camera.start_preview()
    camera.start_recording('/home/pi/Desktop/video.h264')
    sleep(10)
    camera.stop_recording()
    camera.stop_preview()
    camera.close()
    sleep(1)
