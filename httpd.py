# doing a py version of our rb so we can incorporate easil yolo
import sys
import os
import cv2
import numpy as np

from twilio.rest import Client
from b2blaze import B2


from flask import Flask
from flask import request

ACCOUNT_SID = os.environ.get('account_sid')
AUTH_TOKEN  = os.environ.get('auth_token')
B2_ID       = os.environ.get('b2_id')
B2_APP_KEY  = os.environ.get('b2_app_key')
ROOT_PATH   = os.path.dirname(os.path.realpath(__file__))

CLASSES = ['person','bicycle','car','motorcycle','airplane','bus','train','truck','boat','traffic light','fire hydrant','stop sign',
           'parking meter','bench','bird','cat','dog','horse','sheep','cow','elephant','bear','zebra','giraffe','backpack','umbrella',
           'handbag','tie','suitcase','frisbee','skis','snowboard','sports ball','kite','baseball bat','baseball glove','skateboard','surfboard',
           'tennis racket','bottle','wine glass','cup','fork','knife','spoon','bowl','banana','apple','sandwich','orange','broccoli',
           'carrot','hot dog','pizza','donut','cake','chair','couch','potted plant','bed','dining table','toilet','tv','laptop',
           'mouse','remote','keyboard','cell phone','microwave','oven','toaster','sink','refrigerator','book','clock',
           'vase','scissors','teddy bear','hair drier','toothbrush']

COLORS = np.random.uniform(0, 255, size=(len(CLASSES), 3))

NODES = {}# keep track of images captured by node 100 captures per node then recycle
NODES_SEQ_TRIGGERED = {} # keep track of each node and seq pair to avoid rapid fire mms


def get_output_layers(net):
  layer_names = net.getLayerNames()
  
  output_layers = [layer_names[i[0] - 1] for i in net.getUnconnectedOutLayers()]

  return output_layers

def draw_prediction(img, class_id, confidence, x, y, x_plus_w, y_plus_h):
  label = str(CLASSES[class_id])

  color = COLORS[class_id]

  cv2.rectangle(img, (x,y), (x_plus_w,y_plus_h), color, 2)

  cv2.putText(img, label, (x-10,y-10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)

def detect(image_bytes, node, seq):
  #image = cv2.imread(imagedata)
  image = cv2.imdecode(np.frombuffer(image_bytes, np.uint8), -1)

  if image is None or image.any() == False:
     print("Unable to process image bytes! writing error.jpg for inspection...")
     open('error.jpg', 'wb').write(image_bytes)
     return []

  width = image.shape[1]
  height = image.shape[0]
  scale = 0.00392

  net = cv2.dnn.readNet('yolov3.weights', 'yolov3.cfg')

  blob = cv2.dnn.blobFromImage(image, scale, (416,416), (0,0,0), True, crop=False)

  net.setInput(blob)

  outs = net.forward(get_output_layers(net))

  class_ids = []
  confidences = []
  boxes = []
  conf_threshold = 0.5
  nms_threshold = 0.4


  for out in outs:
      for detection in out:
          scores = detection[5:]
          class_id = np.argmax(scores)
          confidence = scores[class_id]
          if confidence > 0.5:
              center_x = int(detection[0] * width)
              center_y = int(detection[1] * height)
              w = int(detection[2] * width)
              h = int(detection[3] * height)
              x = center_x - w / 2
              y = center_y - h / 2
              class_ids.append(class_id)
              confidences.append(float(confidence))
              boxes.append([x, y, w, h])


  indices = cv2.dnn.NMSBoxes(boxes, confidences, conf_threshold, nms_threshold)

  for i in indices:
      i = i[0]
      box = boxes[i]
      x = box[0]
      y = box[1]
      w = box[2]
      h = box[3]
      draw_prediction(image, class_ids[i], confidences[i], round(x), round(y), round(x+w), round(y+h))

  #cv2.imshow("object detection", image)
  #cv2.waitKey()
   
  #cv2.imwrite("object-detection.jpg", image)
  # only capture if one of these 'person','bicycle','car','motorcycle'
  if 0 in class_ids or 1 in class_ids or 2 in class_ids or 3 in class_ids:
    cv2.imwrite( ("public/%s-%d.jpg" % (node, NODES[node])), image)
  #cv2.destroyAllWindows()
  return class_ids

app = Flask(__name__)

@app.route('/')
def hello_world():
  return 'Hello, World!'

@app.route('/capture', methods=['POST'])
def capture():
  node = request.headers.get('X-Node')
  seq  = int(request.headers.get('X-Seq'))

  # allocate 100 file locations per node
  if NODES.get(node, None) == None:
    NODES[node] = 0 # initialize the counter to zero
  NODES[node] += 1

  if NODES[node] > 100:
    NODES[node] = 0

  print("node: %s[%d] - sent data: %d" % (node, seq, len(request.data)))
  image_bytes = request.data
  if image_bytes == None:
    print("bad image data from %s" % node)
    return "Bad Image Data"

  class_ids = detect(image_bytes, node, seq)
  if len(class_ids):
    print("node: %s[%d] detected!" %(node, seq))
    for class_id in class_ids:
      print("%s" % str(CLASSES[class_id]))

  if len(class_ids) > 0:
    print("Interesting Motion")

#    if NODES_SEQ_TRIGGERED.get(node, 0) < seq: # assuming sequence is always incrementing from each node...
#      NODES_SEQ_TRIGGERED[node] = seq
#      print("Alert Motion")
#
#      # TODO: do face detection on the human frames and see if it's a human we know?
#      b2 = B2(key_id=B2_ID, application_key=B2_APP_KEY)
#      bucket = b2.buckets.get('monitors')
#
#      #image_file = open( ("public/%s-%d.jpg" % (node, NODES[node])), 'rb')
#      new_file = bucket.files.upload(contents=image_bytes, file_name='capture/motion.jpg')
#      print(new_file.url)
#
#      client = Client(ACCOUNT_SID, AUTH_TOKEN)
#      message = client.messages.create(to = '+14109806647',
#                                       from_= '+15014564510',
#                                       media_url=[new_file.url],
#                                       body =  "I detected a motion: %s" % new_file.url)
 
  return 'Hello, World!'
