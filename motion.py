import time
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
PIR_PIN = 4 # physical pin 7 and wiring pin 7 BCM 4
GPIO.setup(PIR_PIN, GPIO.IN)

print("PIR Module Test (CTRL+C to exit)")
time.sleep(4)
print("Monitoring motion")

time_of_last_motion = 0

try:
  while(1):
    i = GPIO.input(PIR_PIN)
    print("input: %d" % i)
    if i:
       time_of_motion = time.time()
       time_since_last_motion = time_of_motion - time_of_last_motion
       #print("time_since_last_motion: %d" % time_since_last_motion)
       if time_since_last_motion < 10:
          print(".")
       else:
          print("Motion Detected!")
          time_of_last_motion = time_of_motion

    time.sleep(1)
except KeyboardInterrupt:
  print("Quit")
finally:
  GPIO.cleanup()
