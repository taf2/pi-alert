ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: detector
upload: detector
	pio run -j 2 --target upload && sleep 1;  pio device monitor

detector: targets
	pio run

targets: srcfiles/main.cpp srcfiles/I2SSampler.cpp
	ruby build.rb main.cpp
	ruby build.rb I2SSampler.cpp

monitor:
	pio device monitor

configure:
	pio lib install "TinyPICO Helper Library"

clean:
	pio run --target clean
