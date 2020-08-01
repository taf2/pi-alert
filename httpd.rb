# httpd for handling incoming events from around the network
require 'time'
require 'sinatra'
require "sinatra/json"

# event from a node can include image or video data
# expects json data but can also receive multipart/form-data
# with a file parameter
post '/event' do
  request.body.rewind

  epoch_time = Time.now.to_i
  filename = "./public/#{epoch_time}.jpg"
  File.open(filename, "wb") {|f| f << request.body.read }
  puts "writing #{filename}"

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
