# ---------------------------------------------------------------------
#  Pametna pepeljara — Makefile za avr-gcc 
#
#  make            prevodi projekt i ispisuje zauzeće memorije
#  make flash      upisuje .hex na ploču preko bootloadera
#  make size       samo prikaz zauzeća
#  make clean      briše build direktorij
#
#  Port se zadaje izvana:  make flash PORT=/dev/ttyUSB0
# ---------------------------------------------------------------------

TARGET  = pepeljara
MCU     = atmega328p
F_CPU   = 16000000UL

PORT   ?= /dev/ttyUSB0
BAUD   ?= 115200
PROG   ?= arduino

SRCDIR  = src
BUILD   = build

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(BUILD)/%.o,$(SOURCES))

CXX     = avr-g++
OBJCOPY = avr-objcopy
SIZE    = avr-size

CXXFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) \
           -Os -std=c++14 \
           -Wall -Wextra \
           -fno-exceptions -fno-rtti -fno-threadsafe-statics \
           -ffunction-sections -fdata-sections \
           -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums

LDFLAGS  = -mmcu=$(MCU) -Wl,--gc-sections -Wl,-Map,$(BUILD)/$(TARGET).map

all: $(BUILD)/$(TARGET).hex size

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: $(SRCDIR)/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

size: $(BUILD)/$(TARGET).elf
	@echo ""
	@$(SIZE) --mcu=$(MCU) --format=avr $(BUILD)/$(TARGET).elf

flash: $(BUILD)/$(TARGET).hex
	avrdude -p $(MCU) -c $(PROG) -P $(PORT) -b $(BAUD) \
	        -U flash:w:$(BUILD)/$(TARGET).hex:i

# Serijski terminal za kalibraciju (Ctrl-A pa K za izlaz)
monitor:
	screen $(PORT) 9600

clean:
	rm -rf $(BUILD)

.PHONY: all size flash clean monitor
