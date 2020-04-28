# Web streaming example
# Source code from the official PiCamera package
# http://picamera.readthedocs.io/en/latest/recipes2.html#web-streaming

import io
import picamera
import logging
import socketserver
from threading import Condition
from http import server

PAGE="""\
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">

    <!-- Bootstrap CSS -->
    <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css"
          integrity="sha384-Vkoo8x4CGsO3+Hhxv8T/Q5PaXtkKtu6ug5TOeNV6gBiFeWPGFN9MuhOf23Q9Ifjh" crossorigin="anonymous">

    <title>Raspberry Pi - Surveillance Camera</title>
</head>
<body>
  <header>
      <center><h1>Camera</h1></center>
    </header>
    <main role="main">
      <a class='btn btn-primary' id="watch-video-feed">Check Video Feed</a>
      <center id="video-feed"></center>
    </main>
    <script src="https://code.jquery.com/jquery-3.4.1.slim.min.js"
            integrity="sha384-J6qa4849blE2+poT4WnyKhv5vZF5SrPo0iEjwBvKU7imGFAV0wwj1yYfoRSJoZ+n"
            crossorigin="anonymous"></script>
    <script src="https://cdn.jsdelivr.net/npm/popper.js@1.16.0/dist/umd/popper.min.js"
            integrity="sha384-Q6E9RHvbIyZFJoft+2mJbHaEWldlvI9IOYy5n3zV9zzTtmI3UksdQRVvoxMfooAo"
            crossorigin="anonymous"></script>
    <script src="https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/js/bootstrap.min.js"
            integrity="sha384-wfSDF2E50Y2D1uUdj0O3uMBJnjuUD4Ih7YwaYd1iqfktj0Uod8GCExl3Og8ifwB6"
            crossorigin="anonymous"></script>
    <script>
      $(function() {
        function displayStopFeed() {
          $("#video-feed").html('');
          $('#watch-video-feed').html('Enable Video Feed');
          $("#watch-video-feed").data("processing", false);
        }
        function displayStartFeed() {
          $("#video-feed").html(`<img src="/stream.mjpg" width="640" height="480">`);
          $('#watch-video-feed').html('Disable Video Feed');
          $("#watch-video-feed").data("processing", false);
        }
        $("#watch-video-feed").click(function(e) { e.preventDefault();
          if ($("#watch-video-feed").data("processing")) { return; }
          $("#watch-video-feed").data("processing", true);
          if ($("#video-feed img").length) {
            fetch("/stop_camera", {method: 'POST', body: JSON.stringify({})}).then( () => {
              displayStopFeed();
            }).catch( () => {
              displayStopFeed();
            });
          } else {
            $("#watch-video-feed").html("Loading Video Feed");
            fetch("/start_camera", {method: 'POST', body: JSON.stringify({})}).then( () => {
              setTimeout(displayStartFeed, 5000);
            }).catch( (e) => {
              alert("error loading feed please try again later");
              $('#watch-video-feed').html('Enable Video Feed');
              $("#watch-video-feed").data("processing", false);
            });
          }
        });
      });
    </script>
</body>
</html>
"""

class StreamingOutput(object):
    def __init__(self):
        self.frame = None
        self.buffer = io.BytesIO()
        self.condition = Condition()

    def write(self, buf):
        if buf.startswith(b'\xff\xd8'):
            # New frame, copy the existing buffer's content and notify all
            # clients it's available
            self.buffer.truncate()
            with self.condition:
                self.frame = self.buffer.getvalue()
                self.condition.notify_all()
            self.buffer.seek(0)
        return self.buffer.write(buf)

class StreamingHandler(server.BaseHTTPRequestHandler):
    def do_POST(self):
        print("POST", self.path)
        if self.path == '/start_camera':
            self.send_response(200)
            self.end_headers()
            self.server.start_camera()
            content = '{"status":"success"}'.encode('utf-8')
            self.send_response(200)
            self.send_header('Content-Type', 'text/html')
            self.send_header('Content-Length', len(content))
            self.end_headers()
            self.wfile.write(content)
        elif self.path == '/stop_camera':
            self.send_response(200)
            self.end_headers()
            self.server.stop_camera()
            content = '{"status":"success"}'.encode('utf-8')
            self.send_response(200)
            self.send_header('Content-Type', 'text/html')
            self.send_header('Content-Length', len(content))
            self.end_headers()
            self.wfile.write(content)
        else:
            self.send_error(404)
            self.end_headers()

    def do_GET(self):
        print("GET", self.path)
        if self.path == '/':
            self.send_response(301)
            self.send_header('Location', '/index.html')
            self.end_headers()
        elif self.path == '/index.html':
            content = PAGE.encode('utf-8')
            self.send_response(200)
            self.send_header('Content-Type', 'text/html')
            self.send_header('Content-Length', len(content))
            self.end_headers()
            self.wfile.write(content)
        elif self.path == '/stream.mjpg':
            print("camera: %d", self.server.camera_on)
            if self.server.camera_on == 0:
               print("start camera now")
               self.server.start_camera()
            self.send_response(200)
            self.send_header('Age', 0)
            self.send_header('Cache-Control', 'no-cache, private')
            self.send_header('Pragma', 'no-cache')
            self.send_header('Content-Type', 'multipart/x-mixed-replace; boundary=FRAME')
            self.end_headers()
            try:
                while True:
                    with self.server.output.condition:
                        self.server.output.condition.wait()
                        frame = self.server.output.frame
                    self.wfile.write(b'--FRAME\r\n')
                    self.send_header('Content-Type', 'image/jpeg')
                    self.send_header('Content-Length', len(frame))
                    self.end_headers()
                    self.wfile.write(frame)
                    self.wfile.write(b'\r\n')
            except Exception as e:
                self.server.stop_camera()
                logging.warning('Removed streaming client %s: %s', self.client_address, str(e))
        else:
            self.send_error(404)
            self.end_headers()

class StreamingServer(socketserver.ThreadingMixIn, server.HTTPServer):
    allow_reuse_address = True
    daemon_threads = True
    camera = None
    output = None
    camera_on = 0

    def set_output(self):
        self.output = StreamingOutput()

    def start_camera(self):
        print("start camera")
        if self.camera == None:
           self.camera=picamera.PiCamera(resolution='640x480', framerate=24)
        #Uncomment the next line to change your Pi's Camera rotation (in degrees)
        #camera.rotation = 90
        self.camera.start_recording(self.output, format='mjpeg')
        self.camera_on = 1

    def stop_camera(self):
        print("stop camera")
        self.camera.stop_recording()
        self.camera_on = 0

try:
    address = ('', 8000)
    server = StreamingServer(address, StreamingHandler)
    server.set_output()
    #server.start_camera()
    print("listening on 8000") 
    server.serve_forever()
finally:
    server.server_close()
    server.stop_camera()
