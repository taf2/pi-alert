import RPi.GPIO as GPIO
#from PCA9685 import PCA9685
import time

PIN = 18

GPIO.setmode(GPIO.BCM)
GPIO.setup(PIN, GPIO.OUT)

# is it this https://cdn-shop.adafruit.com/datasheets/PCA9685.pdf
p = GPIO.PWM(PIN, 50)
p.start(0)

current_angle = -1

try: 

  while True:
    angle = float(input("Enter angle between 0 & 180: "))
    if angle < 0 or angle > 180:
      print("invalid input")
      continue

    if current_angle > -1:
      delta = int(angle - current_angle)

      print("change to new angle: %d" % delta)

      if delta == 0:
        print("that's your current angle")
        continue

      # step slowly to the new position
      steps = range(abs(delta))

      for i in steps:
        if delta < 0:
          new_angle = current_angle-i
        else:
          new_angle = current_angle+i

        print("stepping to : %d " % new_angle)

        p.ChangeDutyCycle(2 + (new_angle/18))
        time.sleep(0.1)
        p.ChangeDutyCycle(0)

    else:
      p.ChangeDutyCycle(2 + (angle/18))
      time.sleep(0.5)

    current_angle = angle

    p.ChangeDutyCycle(0)

except EOFError:
  print("\ngood bye")
except KeyboardInterrupt:
  print("\ngood bye")
finally:
  p.stop()
  GPIO.cleanup()
