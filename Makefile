# For Arduino UNO use:
#
#     make MODEL=uno BAUD=115200
#
MODEL=nano:cpu=atmega328
DEVICE=/dev/ttyUSB0
BAUD=57600
BUILDER=arduino-builder -build-path build -fqbn archlinux-arduino:avr:$(MODEL) -hardware /usr/share/arduino/hardware -tools /usr/bin "$$1"
UPLOADER=avrdude -v -patmega328p -carduino -P$(DEVICE) -b$(BAUD) -D -U "flash:w:$$1:i"

all: build/ardecoder.ino.hex

build/ardecoder.ino.hex: ardecoder.ino
	mkdir -p build ; \
	$(subst $$1,$<,$(BUILDER))

upload: build/ardecoder.ino.hex
	$(subst $$1,$<,$(UPLOADER))

clean:
	rm -fr build

.phony: clean
