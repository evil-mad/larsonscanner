#NOTE: This file changes the fuse values of the chip to enable brown-out detection.
#fuse settings are hard-coded into the bottom lines; change them only with care.

PRG            = larsonrunoff
OBJ            = larsonrunoff.o
MCU_TARGET     = attiny2313 
#PROGRAMMER     = avrispmkII 
PROGRAMMER     = usbtiny	#	Must be updated with the type of programmer that you use!
AVRDUDE_TARGET = t2313 
PORT		   = usb

OPTIMIZE       = -Os

DEFS           =
LIBS           =


# You should not have to change anything below here.

CC             = avr-gcc

# Override is only needed by avr-lib build system.

override CFLAGS        = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
override LDFLAGS       = -Wl,-Map,$(PRG).map

OBJCOPY        = avr-objcopy
OBJDUMP        = avr-objdump

all: $(PRG).elf lst text #eeprom

$(PRG).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf *.o $(PRG).elf *.eps *.png *.pdf *.bak *.hex *.bin *.srec
	rm -rf *.lst *.map $(EXTRA_CLEAN_FILES)

lst:  $(PRG).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

# Rules for building the .text rom images

text: hex bin srec

hex:  $(PRG).hex
bin:  $(PRG).bin
srec: $(PRG).srec

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -O srec $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@

# Rules for building the .eeprom rom images

eeprom: ehex ebin esrec


ehex:  $(PRG)_eeprom.hex
#ebin:  $(PRG)_eeprom.bin
esrec: $(PRG)_eeprom.srec

%_eeprom.hex: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@

#%_eeprom.srec: %.elf
#	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O srec $< $@

%_eeprom.bin: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O binary $< $@


# command to program chip (invoked by running "make install")
install: 
	avrdude -p $(AVRDUDE_TARGET) -c $(PROGRAMMER) -P $(PORT) -v -e -b 115200  \
	 -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m \
		 -U flash:w:$(PRG).hex	

# -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m 512 kHz internal clock

# Original fuse settings: 	
#	 -U lfuse:w:0x64:m  -U hfuse:w:0xDF:m	-U efuse:w:0xff:m	
	