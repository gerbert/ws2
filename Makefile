TARG = WS2
MCU = atmega32u4
F_CPU = 16000000

# Includes
INCLUDES  = -I. -I/usr/include/avr -Iinclude -Iusb

# Source files
SRCS = \
	usb/usb_serial.c	\
	timer.c			\
	i2c.c			\
	dht22.c			\
	bmp085.c		\
	ds3231.c		\
	lcd.c			\
	usart.c			\
	nmea.c			\
	main.c

# Compiler options
OPTIMIZE = -Os -mcall-prologues
#CFLAGS   = -g -Wall -Werror -lm $(OPTIMIZE) -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DTWO_LINE_LCD -std=c99 $(INCLUDES)
CFLAGS   = -g -Wall -lm $(OPTIMIZE) -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DTWO_LINE_LCD -std=c99 $(INCLUDES)
CFLAGS	+= -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS	+= -Wundef
#LDFLAGS  = -g -Wall -Werror -mmcu=$(MCU)
LDFLAGS  = -g -Wall -mmcu=$(MCU)

# AVR toolchain and flasher
CC       = avr-gcc
OBJCOPY  = avr-objcopy
OBJDUMP	 = avr-objdump
SIZE	 = avr-size
AVRDUDE  = avrdude
PORT	 = /dev/arduino

OBJS = $(SRCS:.c=.o)

all: $(TARG)

$(TARG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@.elf  $(OBJS) -lm
	$(OBJCOPY) -O ihex -R .eeprom -R .nwram  $@.elf $@.hex
	$(OBJDUMP) -S $@.elf > $@.asm
	$(SIZE) $@.elf
	$(SIZE) --target=ihex $@.hex
#	rm $@.elf

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARG).elf $(TARG).hex $(TARG).asm $(OBJS)

install: flash

flash: $(TARG)
	$(AVRDUDE) -cavr109 -P$(PORT) -p $(MCU) -U flash:w:$(TARG).hex:i
