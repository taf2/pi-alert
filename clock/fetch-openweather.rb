# grab graphics so we can display them in based on the current weather conditions
require 'open-uri'

base_url = 'http://openweathermap.org/img/wn/'#10d@2x.png'

icons = %w(01n 01d 02d 02n 03n 03d 04d 09d 10d 11d 13d 50d 04n 09n 10n 11n 13n 50n)

icons.each {|icon|
  url = "#{base_url}/#{icon}@2x.png"
  puts url
  File.open("icons/#{icon}.png", "wb") {|f|
    open(url, "rb") {|stream| f.write(stream.read) }
  }
}
