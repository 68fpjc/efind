TARGET = efind.x

CROSS = m68k-xelf-
CC = $(CROSS)gcc
AS = $(CROSS)as
LD = $(CROSS)gcc

CFLAGS = -m68000 -O2 -g
LDFLAGS =

.PHONY: all clean

all: $(TARGET)

$(TARGET): main.o
	$(LD) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	-rm -f *.o *.elf* $(TARGET)
