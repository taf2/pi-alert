# This script runs forever reading new data from a redis stream
# we will have a network of detector nodes either raspberry pi or esp32 or anything else that can write to a redis stream
# and expose a captured recording via an http endpoint.
# the job of this process will be to read events from the stream which represent motion detection in a live video feed
# determine if anything interesting was in the stream and save and report what was found
# pip3 install walrus
# see https://github.com/coleifer/walrus/blob/master/examples/work_queue.py example work queue we'll model this after
import sys, traceback

from collections import namedtuple
import os
from functools import wraps
import datetime
import multiprocessing
import json
import time
from walrus import Walrus
import urllib.request
import cv2 
from twilio.rest import Client
from b2blaze import B2


account_sid = os.environ.get('account_sid')
auth_token  = os.environ.get('auth_token')
b2_id       = os.environ.get('b2_id')
b2_app_key  = os.environ.get('b2_app_key')
root        = os.path.dirname(os.path.realpath(__file__))

# TODO: we need a configuration file
ObjDetect_prototxt = "/home/pi/Desktop/MobilNet_SSD_opencv/MobileNetSSD_deploy.prototxt"
ObjDetect_weights  = "/home/pi/Desktop/MobilNet_SSD_opencv/MobileNetSSD_deploy.caffemodel"
ObjDetect_thr      = 0.7

# Labels of Network.
ObjDetect_classNames = { 0: 'background',
    1: 'aeroplane', 2: 'bicycle', 3: 'bird', 4: 'boat',
    5: 'bottle', 6: 'bus', 7: 'car', 8: 'cat', 9: 'chair',
    10: 'cow', 11: 'diningtable', 12: 'dog', 13: 'horse',
    14: 'motorbike', 15: 'person', 16: 'pottedplant',
    17: 'sheep', 18: 'sofa', 19: 'train', 20: 'tvmonitor' }

# Lightweight wrapper for storing exceptions that occurred executing a task.
TaskError = namedtuple('TaskError', ('error',))

class TaskQueue(object):
    def __init__(self, client, stream_key='tasks'):
        self.client = client  # our Redis client.
        self.stream_key = stream_key

        # We'll also create a consumer group (whose name is derived from the
        # stream key). Consumer groups are needed to provide message delivery
        # tracking and to ensure that our messages are distributed among the
        # worker processes.
        self.name = stream_key + '-cg'
        self.consumer_group = self.client.consumer_group(self.name, stream_key)
        self.result_key = stream_key + '.results'  # Store results in a Hash.

        # Obtain a reference to the stream within the context of the
        # consumer group.
        self.stream = getattr(self.consumer_group, stream_key)
        self.signal = multiprocessing.Event()  # Used to signal shutdown.
        self.signal.set()  # Indicate the server is not running.

        # Create the stream and consumer group (if they do not exist).
        self.consumer_group.create()
        self._running = False
        self._tasks = {}  # Lookup table for mapping function name -> impl.

    def task(self, fn):
        self._tasks[fn.__name__] = fn  # Store function in lookup table.

        @wraps(fn)
        def inner(*args, **kwargs):
            # When the decorated function is called, a message is added to the
            # stream and a wrapper class is returned, which provides access to
            # the task result.
            message = self.serialize_message(fn, args, kwargs)

            # Our message format is very simple -- just a "task" key and a blob
            # of json data. You could extend this to provide additional
            # data, such as the source of the event, etc, etc.
            task_id = self.stream.add({'task': message})
            return TaskResultWrapper(self, task_id)
        return inner

    def deserialize_message(self, message):
        task_name, args = json.loads(message)
        if task_name not in self._tasks:
            raise Exception('task "%s" not registered with queue.')
        print("task_name: %s" % task_name)
        return self._tasks[task_name], args

    def serialize_message(self, task, args=None, kwargs=None):
        return json.dumps((task.__name__, args, kwargs))

    def store_result(self, task_id, result):
        # API for storing the return value from a task. This is called by the
        # workers after the execution of a task.
        if result is not None:
            pipe = self.client.pipeline()
            pipe.hset(self.result_key, task_id, json.dumps(result))
            pipe.expire(self.result_key, 28800) # expire results in 8 hours
            pipe.execute()

    def get_result(self, task_id):
        # Obtain the return value of a finished task. This API is used by the
        # TaskResultWrapper class. We'll use a pipeline to ensure that reading
        # and popping the result is an atomic operation.
        pipe = self.client.pipeline()
        pipe.hexists(self.result_key, task_id)
        pipe.hget(self.result_key, task_id)
        pipe.hdel(self.result_key, task_id)
        exists, val, n = pipe.execute()
        return json.loads(val) if exists else None

    def run(self, nworkers=1):
        if not self.signal.is_set():
            raise Exception('workers are already running')

        # Start a pool of worker processes.
        self._pool = []
        self.signal.clear()
        for i in range(nworkers):
            worker = TaskWorker(self)
            worker_t = multiprocessing.Process(target=worker.run)
            worker_t.start()
            self._pool.append(worker_t)

    def shutdown(self):
        if self.signal.is_set():
            raise Exception('workers are not running')

        # Send the "shutdown" signal and wait for the worker processes
        # to exit.
        self.signal.set()
        for worker_t in self._pool:
            worker_t.join()


class TaskWorker(object):
    _worker_idx = 0

    def __init__(self, queue):
        self.queue = queue
        self.consumer_group = queue.consumer_group

        # Assign each worker processes a unique name.
        TaskWorker._worker_idx += 1
        worker_name = 'worker-%s' % TaskWorker._worker_idx
        self.worker_name = worker_name

    def run(self):
        print("TaskWorker ready and waiting for work")
        while not self.queue.signal.is_set():
            # Read up to one message, blocking for up to 1sec, and identifying
            # ourselves using our "worker name".
            resp = self.consumer_group.read(1, 1000, self.worker_name)
            if resp is not None:
                # Resp is structured as:
                # {stream_key: [(message id, data), ...]}
                for stream_key, message_list in resp:
                    task_id, data = message_list[0]
                    print("received task: %s with data %s" % (task_id, data))
                    self.execute(task_id.decode('utf-8'), data[b'task'])

    def execute(self, task_id, message):
        print("received task: %s with data %s" % (task_id, message))
        # Deserialize the task message, which consists of the task name, args
        # and kwargs. The task function is then looked-up by name and called
        # using the given arguments.
        try: 
          task, args = self.queue.deserialize_message(message)
          try:
              ret = task(args)
          except Exception as exc:
              print("oh no what? %s" % str(exc))
              traceback.print_exc(file=sys.stdout)

              # On failure, we'll store a special "TaskError" as the result. This
              # will signal to the user that the task failed with an exception.
              self.queue.store_result(task_id, TaskError(str(exc)))
          else:
              # Store the result and acknowledge (ACK) the message.
              self.queue.store_result(task_id, ret)
              self.queue.stream.ack(task_id)
        except Exception as exc:
          print("oh no what? %s" % str(exc))
          traceback.print_exc(file=sys.stdout)

          # parse error just ack  and move on
          self.queue.stream.ack(task_id)


class TaskResultWrapper(object):
    def __init__(self, queue, task_id):
        self.queue = queue
        self.task_id = task_id
        self._result = None

    def __call__(self, block=True, timeout=None):
        if self._result is None:
            # Get the result from the result-store, optionally blocking until
            # the result becomes available.
            if not block:
                result = self.queue.get_result(self.task_id)
            else:
                start = time.time()
                while timeout is None or (start + timeout) > time.time():
                    result = self.queue.get_result(self.task_id)
                    if result is None:
                        time.sleep(0.1)
                    else:
                        break

            if result is not None:
                self._result = result

        if self._result is not None and isinstance(self._result, TaskError):
            raise Exception('task failed: %s' % self._result.error)

        return self._result

if __name__ == '__main__':
    db = Walrus()  # roughly equivalent to db = Redis().
    queue = TaskQueue(db)

    @queue.task
    def object_detect(task_data):
      node_name      = task_data['name']
      node_video_url = task_data['url']

      # fetch from the node's ip the video file
      # use opencv to do object detection
      downloaded_video_path = "/tmp/video.mp4"

      with urllib.request.urlopen(node_video_url) as response:
        download_file_handle = open(downloaded_video_path, "wb")
        download_file_handle.write(response.read())
        download_file_handle.close()

      fourcc = cv2.VideoWriter_fourcc(*'mp4v')
      cap = cv2.VideoCapture(downloaded_video_path)
      net = cv2.dnn.readNetFromCaffe(ObjDetect_prototxt, ObjDetect_weights)
      out = None;

      detected_human = 0
      detected_car = 0

      # this is a rather time consuming high cpu work load so to reduce this we'll only 
      # scan upto max_check_frames and we'll try to evenly distribute the checks to frames such that we analysis
      # a good amount of the 10-12 second video clip capture that was sent to us...
      frame_count = 0
      check_interval = 20 # check every check_interval frame e.g. frame_count % check_interval == 1
      checked_frames = 0
      max_check_frames = 10

      try:
        while True:
          # Capture frame-by-frame
          ret, frame = cap.read()
          if ret != True:
            break


          frame_count += 1
          if frame_count % check_interval == 0 and checked_frames < max_check_frames:
            print("check frames: %d" % frame_count)
            checked_frames += 1

            frame_resized = cv2.resize(frame,(300,300)) # resize frame for prediction

            # MobileNet requires fixed dimensions for input image(s)
            # so we have to ensure that it is resized to 300x300 pixels.
            # set a scale factor to image because network the objects has differents size. 
            # We perform a mean subtraction (127.5, 127.5, 127.5) to normalize the input;
            # after executing this command our "blob" now has the shape:
            # (1, 3, 300, 300)
            blob = cv2.dnn.blobFromImage(frame_resized, 0.007843, (300, 300), (127.5, 127.5, 127.5), False)
            #Set to network the input blob 
            net.setInput(blob)
            #Prediction of network
            detections = net.forward()

            #Size of frame resize (300x300)
            cols = frame_resized.shape[1] 
            rows = frame_resized.shape[0]

            #For get the class and location of object detected, 
            # There is a fix index for class, location and confidence
            # value in @detections array .
            for i in range(detections.shape[2]):
              confidence = detections[0, 0, i, 2] #Confidence of prediction 
              if confidence > ObjDetect_thr: # Filter prediction 
                class_id = int(detections[0, 0, i, 1]) # Class label

                # Object location 
                xLeftBottom = int(detections[0, 0, i, 3] * cols) 
                yLeftBottom = int(detections[0, 0, i, 4] * rows)
                xRightTop   = int(detections[0, 0, i, 5] * cols)
                yRightTop   = int(detections[0, 0, i, 6] * rows)
                
                # Factor for scale to original size of frame
                heightFactor = frame.shape[0]/300.0  
                widthFactor = frame.shape[1]/300.0 
                # Scale object detection to frame
                xLeftBottom = int(widthFactor * xLeftBottom) 
                yLeftBottom = int(heightFactor * yLeftBottom)
                xRightTop   = int(widthFactor * xRightTop)
                yRightTop   = int(heightFactor * yRightTop)
                # Draw location of object  
                cv2.rectangle(frame, (xLeftBottom, yLeftBottom), (xRightTop, yRightTop),
                              (0, 255, 0))

                # Draw label and confidence of prediction in frame resized
                if class_id in ObjDetect_classNames:
                  label = ObjDetect_classNames[class_id] + ": " + str(confidence)
                  labelSize, baseLine = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)

                  yLeftBottom = max(yLeftBottom, labelSize[1])
                  cv2.rectangle(frame, (xLeftBottom, yLeftBottom - labelSize[1]),
                                       (xLeftBottom + labelSize[0], yLeftBottom + baseLine),
                                       (255, 255, 255), cv2.FILLED)
                  cv2.putText(frame, label, (xLeftBottom, yLeftBottom),
                              cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0))

                  print(label) #print class and confidence
                  # person: 0.9904131
                  if ObjDetect_classNames[class_id] == 'person':
                    detected_human = 1

                  if ObjDetect_classNames[class_id] == 'car':
                    detected_car = 1

            #cv2.namedWindow("frame", cv2.WINDOW_NORMAL)
            #cv2.imshow("frame", frame)
          #else:
          #  print("skip analysis of frame: %d" % frame_count)

          if out == None:
            fshape = frame.shape
            print("init frame output")
            fheight = fshape[0]
            fwidth = fshape[1]
            print(fwidth, fheight)

            out = cv2.VideoWriter('/tmp/video-boxed.mp4', fourcc, 25.0, (fwidth,fheight))

          out.write(frame)

          #if cv2.waitKey(1) >= 0:  # Break with ESC 
          #    break
      except KeyboardInterrupt:
        print("Quit")
      finally:
        print("cleaning up input and output")
        out.release()
        cap.release()

      # if we detected a human or car alert
      # archive all recordings (we need that ssd drive)
      print("finished analysis : %d %d" % (detected_human, detected_car))

      if detected_human == 1:
        print("Alert a human entered")
        # TODO: do face detection on the human frames and see if it's a human we know?
        b2 = B2(key_id=b2_id, application_key=b2_app_key)
        bucket = b2.buckets.get('monitors')
        os.system("/usr/bin/ffmpeg -y -i /tmp/video-boxed.mp4 -vf scale=320:240 /tmp/video-sized.mp4")

        image_file = open('/tmp/video-sized.mp4', 'rb')
        new_file = bucket.files.upload(contents=image_file, file_name='capture/person.mp4')
        print(new_file.url)
        image_file.close()

        client = Client(account_sid, auth_token)
        message = client.messages.create(to = '+14109806647',
                                         from_= '+15014564510',
                                         media_url=[new_file.url],
                                         body =  "I detected a human: %s" % new_file.url)
      if detected_car == 1:
        print("Alert a car entered")
        # TODO: is it one of our cars?
        b2 = B2(key_id=b2_id, application_key=b2_app_key)
        bucket = b2.buckets.get('monitors')
        os.system("/usr/bin/ffmpeg -y -i /tmp/video-boxed.mp4 -vf scale=320:240 /tmp/video-sized.mp4")
        # ffmpeg -i output.mp4 -vf scale=320:240 output_320.mp4

        image_file = open('/tmp/video-sized.mp4', 'rb')
        new_file = bucket.files.upload(contents=image_file, file_name='capture/car.mp4')
        print(new_file.url)
        image_file.close()

        client = Client(account_sid, auth_token)
        message = client.messages.create(to = '+14109806647',
                                         from_= '+15014564510',
                                         media_url=[new_file.url],
                                         body =  "I detected a car: %s" % new_file.url)

      print("job done") 

    # start with 1 job processor for now
    try:
      queue.run(1)
      print("main thread idling...")
      while True:
        time.sleep(1) # wait for jobs
    except KeyboardInterrupt:
      print("Quit")
    finally:
      queue.shutdown()
      print('done!')
