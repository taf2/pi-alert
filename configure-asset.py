import os
from twilio.rest import Client

account_sid = os.environ.get('account_sid')
service_sid = os.environ.get('service_sid')
auth_token  = os.environ.get('auth_token')

client = Client(account_sid, auth_token)

print("creating asset sid")

asset = client.serverless.services(service_sid).assets.create(friendly_name='For MMS Short Lived')

print("export your asset sid for updating: export asset_sid=", asset.sid)
