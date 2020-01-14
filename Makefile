BUILDER=arduino-builder -build-path build -fqbn archlinux-arduino:avr:nano:cpu=atmega328 -hardware /usr/share/arduino/hardware -tools /usr/bin
UPLOADER=./uploader
DEVICE=/dev/ttyUSB0

all: build/ardecoder.ino.hex

build/ardecoder.ino.hex: ardecoder.ino
	mkdir -p build ; \
	$(BUILDER) ardecoder.ino

upload: build/ardecoder.ino.hex
	$(UPLOADER) $<

clean:
	rm -fr build

.phony: clean
