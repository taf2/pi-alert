import os
from twilio.rest import Client

account_sid = os.environ.get('account_sid')
auth_token  = os.environ.get('auth_token')

client = Client(account_sid, auth_token)


print("setting up service for env")

service = client.serverless.services.create(include_credentials=True, unique_name='taf2-camera', friendly_name='Assets for Camera')

print("add the following 'export service_sid=", service.sid)
print(service.sid)
