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
