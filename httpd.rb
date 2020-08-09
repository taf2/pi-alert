# httpd for handling incoming events from around the network
require 'time'
require 'sinatra'
require "sinatra/json"
require 'dlib'
require 'thread'

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

# event from a node can include image or video data
# expects json data but can also receive multipart/form-data
# with a file parameter
post '/event' do
  request.body.rewind
  if counter  > 100
    counter = 0
  end

  epoch_time = Time.now.to_i
  filename = "./public/#{counter}.jpg"
  counter += 1

  puts "writing #{filename}"
  File.open(filename, "wb") {|f| f << request.body.read }

  detection(filename)

  # respond with json including the time of day
  json({
    epoch_time: epoch_time
  })
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
