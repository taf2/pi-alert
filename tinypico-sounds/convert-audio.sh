# helps to isolate speach 
ffmpeg -f s16le -sample_rate 44100 -i audio.raw -af "highpass=f=200, lowpass=f=3000" -f wav file-out.wav
