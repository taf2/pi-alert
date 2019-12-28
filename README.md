# Pi Alert
 You will need a Backblaze account and a Twilio account.  You'll need a raspberrypi with a PIR motion sensor and a camera

# Install
sudo apt-get -y install build-essential cmake cmake-qt-gui pkg-config libpng12-0 libpng12-dev libpng++-dev libpng3 libpnglite-dev zlib1g-dbg zlib1g zlib1g-dev pngtools libtiff4-dev libtiff4 libtiffxx0c2 libtiff-tools
sudo apt-get install python-opencv
pip install python-pip
pip install b2blaze
pip install twilio

# update
recent version of pi we had to install with:

sudo apt-get -y install build-essential cmake cmake-qt-gui pkg-config libpng12-0 libpng12-dev libpng12++-dev libpnglite-dev zlib1g-dbg zlib1g zlib1g-dev pngtools libtiff-dev libtiff-tools


you may also need
sudo apt-get install python3-picamera
