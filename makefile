PROGRAM = efind
VERSION = 0.1.0

TARGET = $(PROGRAM).x
ARCHIVE = $(PROGRAM)-$(VERSION).zip

CROSS = m68k-xelf-
CC = $(CROSS)gcc
AS = $(CROSS)as
LD = $(CROSS)gcc

# CFLAGS = -m68000 -O0 -Wall -MMD -DVERSION=\"${VERSION}\" -g
CFLAGS = -m68000 -O3 -Wall -DVERSION=\"${VERSION}\" -MMD
LDFLAGS =
OBJS = main.o
LDLIBS =

.PHONY: all arc clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ $(LDLIBS) -o $@

README.txt: README.md
	pandoc -f markdown -t plain $< >$@

arc: $(ARCHIVE)

$(ARCHIVE): $(TARGET) README.txt
	7z a $(PROGRAM)-$(VERSION).zip $^

DEPS = $(patsubst %.o,%.d,$(OBJS))

clean:
	-rm -f *.x *.o *.elf* *.d README.txt *.zip
