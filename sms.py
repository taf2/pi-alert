import os
from twilio.rest import Client

account_sid = os.environ.get('account_sid')
auth_token  = os.environ.get('auth_token')

client = Client(account_sid, auth_token)

message = client.messages.create(to = '+14109806647',
                                 from_= '+15014564510',
                                 body =  "We can do this")

