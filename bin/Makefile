CC	= gcc
CFLAGS	+= -std=gnu99 -Wall -pedantic -g -D_LARGEFILE64_SOURCE=1
LDFLAGS	+= -lrt

all: mkfs.lanyfs detectfs.lanyfs

mkfs.lanyfs: mkfs.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $<

detectfs.lanyfs: detectfs.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -rf mkfs.lanyfs detectfs.lanyfs mkfs.o detectfs.o

.PHONY: clean

