PORT=COM5
MCU=atmega32u4
CFLAGS=-DF_CPU=16000000UL -g -Wall -mcall-prologues -mmcu=$(MCU) -Os --param=min-pagesize=0 # TODO: Need to check on this last flag param=min-pagesize. 
# It appaers to be a bug in GCC version 12.
LDFLAGS=-Wl,-gc-sections -Wl,-relax
CC=avr-gcc
TARGET=main
OBJECT_FILES=main.o led.o usb.o uart.o

all: $(TARGET).hex

clean:
	rm -f *.o *.hex *.obj *.hex

%.hex: %.obj
	avr-objcopy -R .eeprom -O ihex $< $@

%.obj: $(OBJECT_FILES)
	$(CC) $(CFLAGS) $(OBJECT_FILES) $(LDFLAGS) -o $@

program: $(TARGET).hex
	avrdude -p $(MCU) -c avr109 -P $(PORT) -U flash:w:$(TARGET).hex