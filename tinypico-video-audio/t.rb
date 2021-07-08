require 'thread'
require 'sinatra'

audio_channel = Queue.new

processor = Thread.new do
  puts "audio thread started"
  total_captured = 0
  last_capture = nil
  while true
    captured_audio = 0
    while !audio_channel.empty?
      printf("x")
      audio_buffer = audio_channel.pop
      File.open("audio.raw", "ab") {|f|
        f << audio_buffer
      }
      captured_audio += 1
    end
    last_capture = Time.now.to_i if last_capture.nil? && captured_audio > 0
    total_captured += captured_audio
    if !last_capture.nil?
      time_since_capture_start = (Time.now.to_i - last_capture.to_i).to_i
      puts "time since capture start: #{time_since_capture_start} seconds"
    end
    if total_captured > 100 || (total_captured > 0 && time_since_capture_start > 30) # either more then 100 captured samples in buffer or 30 seconds of time
      last_capture = nil
      seconds_of_audio_captured = 0
      total_captured = 0
      puts "\tencoding captured #{captured_audio} audio buffers\n"
      # play the new audio
      system(%(ffmpeg -f s16le -sample_rate 24000 -i audio.raw -af 'highpass=f=200, lowpass=f=3000' -f wav file-out.wav -y))
      #system(%(ffmpeg -f s16le -sample_rate 16000 -i audio.raw -af 'highpass=f=200, lowpass=f=3000' -f wav file-out.wav -y))
      system("afplay -d  file-out.wav")
      File.unlink("audio.raw")
      File.unlink("file-out.wav")
      sleep 5 # wake up every few seconds to transcode some audio
    else
      printf(".")
      sleep 1 # wake up every few seconds to transcode some audio
    end
  end
  puts "end thread"
end

set :port, 5003

post '/samples' do
  audio_channel << request.body.read
	#File.open("audio.raw", "ab") {|f|
		#f << request.body.read
	#}
  #audio << Time.now.to_i
	[200, 'OK']
end

count = 0
post '/capture' do
	File.open("video#{count}.jpg", "wb") {|f|
		f << request.body.read
	}
  count += 1
  #audio << Time.now.to_i
	[200, 'OK']
end
