TARGET = efind.x

CROSS = m68k-xelf-
CC = $(CROSS)gcc
AS = $(CROSS)as
LD = $(CROSS)gcc

# CFLAGS = -m68000 -O0 -g -MMD
CFLAGS = -m68000 -O3 -MMD
LDFLAGS =
OBJS = main.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ $(LDLIBS) -o $@

DEPS = $(patsubst %.o,%.d,$(OBJS))

clean:
	-rm -f *.o *.elf* *.d $(TARGET)
