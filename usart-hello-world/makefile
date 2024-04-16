MCU = atmega32u4
CC = avr-gcc
OBJCOPY = avr-objcopy
CFLAGS = -mmcu=$(MCU) -Wall

build:
	$(CC) $(CFLAGS) -o main.elf main.c
	$(OBJCOPY) -O binary main.elf main.bin

upload: build
	@avrdude -c usbasp -p atmega32u4 -U flash:w:main.bin:r

clean:
	rm -f main.elf main.bin
