# httpd for handling incoming events from around the network
require 'time'
require 'sinatra'
require "sinatra/json"

# event from a node can include image or video data
# expects json data but can also receive multipart/form-data
# with a file parameter
post '/event' do
  @filename = params.dig(:file,:filename)
  file = params.dig(:file,:tempfile)
  if file
    puts "received a file write to public as #{@filename.inspect}"
    # TODO
    File.open("./public/#{@filename}", 'wb') do |f|
      f.write(file.read)
    end
  end

  # respond with json including the time of day
  json({
    epoch_time: Time.now.to_i
  })
end

get '/time' do
  json epoch_time: Time.now.to_i
end
