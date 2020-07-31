ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: detector
upload: detector
	pio run --target upload && sleep 1;  pio device monitor

detector: targets
	pio run

targets: srcfiles/main.cpp
	ruby build.rb main.cpp

monitor:
	pio device monitor

install_decoder:
	cd ${ROOT_DIR} && git clone https://github.com/littleyoda/EspStackTraceDecoder.git && cd ${ROOT_DIR}/EspStackTraceDecoder && ant

#
# dependencies:
# make install_decoder
# COREFILE is a path to the core stacktrace data you can capture while monitoring the running program
#
# usage:
# 
# make decode COREFILE=esp.core
#
decode:
	java -jar ${ROOT_DIR}/EspStackTraceDecoder/build/release/EspStackTraceDecoder.jar ~/.platformio/packages/toolchain-xtensa/bin/xtensa-lx106-elf-addr2line .pio/build/d1/firmware.elf ${COREFILE} #esp.core

configure:
	pio lib install "ArduCAM"

clean:
	pio run --target clean
