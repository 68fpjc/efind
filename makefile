VERSION = 0.1.0

TARGET = efind.x

CROSS = m68k-xelf-
CC = $(CROSS)gcc
AS = $(CROSS)as
LD = $(CROSS)gcc

# CFLAGS = -m68000 -O0 -Wall -MMD -DVERSION=\"${VERSION}\" -g
CFLAGS = -m68000 -O3 -Wall -DVERSION=\"${VERSION}\" -MMD
LDFLAGS =
OBJS = main.o
LDLIBS =

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ $(LDLIBS) -o $@

DEPS = $(patsubst %.o,%.d,$(OBJS))

clean:
	-rm -f *.x *.o *.elf* *.d
