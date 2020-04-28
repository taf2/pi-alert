# send a picture from the raspi camera to my phone number
import os
import time
import picamera

from twilio.rest import Client
from b2blaze import B2

b2_id       = os.environ.get('b2_id')
b2_app_key  = os.environ.get('b2_app_key')

account_sid = os.environ.get('account_sid')
auth_token  = os.environ.get('auth_token')

client = Client(account_sid, auth_token)

with picamera.PiCamera() as camera:
    camera.capture_sequence(['image1.jpg',])
    camera.close()

b2 = B2(key_id=b2_id, application_key=b2_app_key)
bucket = b2.buckets.get('monitors')

image_file = open('image1.jpg', 'rb')
new_file = bucket.files.upload(contents=image_file, file_name='capture/result.jpg')
print(new_file.url)

message = client.messages.create(to = '+14109806647',
                                 from_= '+15014564510',
                                 media_url=[new_file.url],
                                 body =  "We can do this")
