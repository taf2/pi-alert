# httpd for handling incoming events from around the network
require 'time'
require 'sinatra'
require "sinatra/json"
#require 'dlib'
require 'curb'
require 'json'
require 'thread'
require 'redis'

RS = Redis.new(host:"192.168.2.50")

JOBS = Queue.new
JOB_WORKER = Thread.new do
  while (work=JOBS.pop)
    # notify the front porch
    Curl.get("http://192.68.2.75:5000/") {|http|
      http.timeout = 20
    }
  end
end

class Httpd < Sinatra::Base


set :server_settings, :timeout => 60

counter = Dir['./public/*.jpg'].size

def detection(image_file)
  img = Dlib::Image.load(image_file)
  detector = Dlib::FrontalFaceDetector.new

  rects = detector.detect(img)

  puts "number of faces detected #{rects.length}"
  rects.each.with_index do |rect, index|
    puts "Detection #{index}: Left: #{rect.left} Top: #{rect.top} Right: #{rect.right} Bottom: #{rect.bottom} Area: #{rect.area}"
    img.draw_rectangle!(rect, [0, 0, 255]) if rect.area > 0
  end

  output_file = image_file.gsub(/\.jpg/,'-faces.jpg')
  img.save_jpeg(output_file)
end

post '/event' do
  JOBS << "event" # TODO: put the name of the device here so we know who to notify to activate
end

# event from a node can include image or video data
# expects json data but can also receive multipart/form-data
# with a file parameter
post '/capture' do
  puts "receved capture request"
  request.body.rewind
  if counter  > 100
    counter = 0
  end

  epoch_time = Time.now.to_i
  filename = "./public/#{counter}.jpg"

  puts "writing #{filename}"
  File.open(filename, "wb") {|f| f << request.body.read }

  #detection(filename)
  # instead of doing detection here we'll just relay to our detection worker process so we'll add this to redis stream
  # for processing
  # run redis-cli -h 192.168.2.50 XADD tasks '*' task '["object_detect", {"name": "frontporch", "url": "http://192.168.2.75/video0.h264"}]'
#  event = ["object_detect", {"type":"jpg", "name": "frontdrive", "url": "http://192.168.2.120:4567/video#{counter}.jpg"}]
#  RS.xadd("tasks", {task: event.to_json})
#  system(%(redis-cli -h 192.168.2.50 XADD tasks '*' task '["object_detect", ]))

  counter += 1

  # respond with json including the time of day
  json({
    epoch_time: epoch_time
  })
end

# so the redis stream processing job server can fetch this work
get '/video:id.jpg' do
  #File.read("./public/#{params[:id]}.jpg")
  send_file("./public/#{params[:id]}.jpg")
end


get '/' do
%(
<form action="/event" method="POST" enctype="multipart/form-data">
  <input type="file" name="file"/>
  <input type="submit" value="go"/>
</form>
)
end

get '/time' do
  json epoch_time: Time.now.to_i
end

end
