import os
from b2blaze import B2
b2_id       = os.environ.get('b2_id')
b2_app_key  = os.environ.get('b2_app_key')

b2 = B2(key_id=b2_id, application_key=b2_app_key)
bucket = b2.buckets.get('monitors')

image_file = open('result.jpg', 'rb')
new_file = bucket.files.upload(contents=image_file, file_name='capture/result.jpg')
print(new_file.url)
