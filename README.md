# Definitely don't use any of this it's just a bunch of rubish


# Pi Alert
 You will need a Backblaze account and a Twilio account.  You'll need a raspberrypi with a PIR motion sensor and a camera

# Install
sudo apt-get -y install build-essential cmake cmake-qt-gui pkg-config libpng12-0 libpng12-dev libpng++-dev libpng3 libpnglite-dev zlib1g-dbg zlib1g zlib1g-dev pngtools libtiff4-dev libtiff4 libtiffxx0c2 libtiff-tools
sudo apt-get install python-opencv
pip3 install python-pip
pip3 install b2blaze
pip3 install twilio

# update
recent version of pi we had to install with:

sudo apt-get -y install build-essential cmake cmake-qt-gui pkg-config libpng12-0 libpng12-dev libpng12++-dev libpnglite-dev zlib1g-dbg zlib1g zlib1g-dev pngtools libtiff-dev libtiff-tools
sudo apt-get install python3-opencv
sudo pip3 install opencv-python
sudo apt-get install libcblas-dev
sudo apt-get install libhdf5-dev
sudo apt-get install libhdf5-serial-dev
sudo apt-get install libatlas-base-dev
sudo apt-get install libjasper-dev
sudo apt-get install libqtgui4
sudo apt-get install libqt4-test
sudo apt-get install python3-picamera

service is stream-http.py

# Moving to all C and monitor.c and a client server setup

# deps
sudo apt install libhiredis-dev
sudo apt install ffmpeg

# design
We are a single node in a many node system our master node/s will have redis (sentinels)

When we detect an object or a sensor is triggered we'll do some LED indicators for 
on site troubleshooting but our primary focus is to capture video.  We'll record say
10 seconds on trigger and then we'll use our redis connection to write into a redis
stream

Our master will read from the stream which will tell it to grab files from our local 
nginx server.  We'll keep a range of files and they'll just loop overwriting them over
time as to keep a constant disk on the nodes


# servos
sudo apt-get install libffi-dev
pip3 install PCA9685-driver

# PI Audio with USB

https://makersportal.com/blog/2018/8/23/recording-audio-on-the-raspberry-pi-with-python-and-a-usb-microphone
## maybe using gstreamer
and something like:
  gst-launch-1.0 -e autovideosrc ! queue ! videoconvert ! mkv. autoaudiosrc ! queue ! audioconvert ! mkv. matroskamux name=mkv ! filesink location=test.mkv sync=false
  # with the USB mic 

sudo apt-get install libx264-dev libjpeg-dev ibgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-ugly gstreamer1.0-tools
sudo apt-get install gstreamer1.0-plugins-bad
sudo apt-get install gstreamer1.0-alsa
sudo apt-get install gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-libav

interesting https://raspberrypi.stackexchange.com/questions/25962/sync-audio-video-from-pi-camera-usb-microphone

# nice: https://www.reddit.com/r/raspberry_pi/comments/nfjc1n/i_finally_figured_out_how_to_stream_from/
ffmpeg  -video_size 1280x720 -i /dev/video0 \
        -f alsa -channels 1 -sample_rate 44100 -i hw:1 \
        -vf "drawtext=text='%{localtime}': x=(w-tw)/2: y=lh: fontcolor=white: fontsize=24" \
        -af "volume=15.0" \
        -c:v h264_omx -b:v 2500k \
        -c:a libmp3lame -b:a 128k \
        -map 0:v -map 1:a -f flv out.mp4
