# Clock

An ePaper clock with a music alarm configured via Bluetooth low energy using a ItsyBitsy M4 and a feather ESP32

![ePaper Clock](https://raw.githubusercontent.com/taf2/pi-alert/master/clock/parts/Clock%20v27.png)
[Video of the Clock](https://www.youtube.com/watch?v=r2SdpJf9KkA)

# Repos
* display-feather - ESP32 controller for BLE and WiFi (main source of power)
* epaper7color    - ItsyBitsy M4 driver for ePaper
* ../actitapp     - web app for ble configuration UI

# Components

* [Adafruit HUZZAH32 – ESP32 Feather Board](https://www.adafruit.com/product/3405)
* [Adafruit ItsyBitsy M4](https://www.adafruit.com/product/3800)
* [DFPlayer - A Mini MP3 Player](https://www.dfrobot.com/product-1121.html)
* [5.65inch ACeP 7-Color E-Paper E-Ink Display Module, 600×448 Pixels](https://www.waveshare.com/5.65inch-e-paper-module-f.htm)
* [Adafruit MiniBoost 5V @ 1A - TPS61023](https://www.adafruit.com/product/4654)
* [4700uF 10v Electrolytic Capacitor](https://www.adafruit.com/product/1589)
* [Lithium Ion Polymer Battery - 3.7v 2500mAh]( https://www.adafruit.com/product/328)
* [Adafruit Perma-Proto Half-sized Breadboard PCB - Single](https://www.adafruit.com/product/1609)
* [Mini Oval Speaker - 8 Ohm 1 Watt](https://www.adafruit.com/product/3923)
* blue led for internal indicator for BLE with a resistor


# Preparing the images

Trying to figure out the available colors from https://www.waveshare.com/5.65inch-e-paper-module-f.htm

back
white
8f1c1f red
216627 green
1a1d52 blue
d4c93b yellow
c27831 orange

Using ImageMagick via https://learn.adafruit.com/preparing-graphics-for-e-ink-displays?view=all#command-line

# Resolution
600 x 448
