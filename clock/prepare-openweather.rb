# prepare open weather images after running fetch-openweather.rb
# using ImageMagick
# convert input.jpg -dither FloydSteinberg -define dither:diffusion-amount=85% -remap eink-3color.png -type truecolor BMP3:output.bmp
# curl 'https://cdn-learn.adafruit.com/assets/assets/000/079/736/large1024/eink___epaper_eink-3color.png?1566418646' > eink-3color.png

Dir["icons/*.png"].each {|icon|
  bmp = icon.gsub(/\.png$/,'.bmp')
  puts "converting: #{icon.inspect} to #{bmp.inspect}"

  # for 3 color eink
  #system("convert #{icon} -dither FloydSteinberg -define dither:diffusion-amount=85% -remap eink-3color.png -type truecolor BMP3:#{bmp}")

  # for 7 color eink see pallet.js
  system("convert #{icon} -dither FloydSteinberg -define dither:diffusion-amount=85% -remap eink-7color.png -type truecolor BMP3:#{bmp}")
}

Dir["icons/*.bmp"].each {|bmp|
  name = File.basename(bmp).split(".").first
  system("node converter.js -f arduino -m horizontal565 -b transparent -n icon#{name} -i '#{bmp}' -o epaper7color/src/#{name}.h")
}
