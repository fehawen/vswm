LIBS = -lX11
CFLAGS += -std=c99 -Wall -Wextra -pedantic -Os
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
CC ?= gcc

all: flowm

flowm: flowm.o
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -f flowm $(DESTDIR)$(BINDIR)
	chmod 755 $(DESTDIR)$(BINDIR)/flowm

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/flowm

clean:
	rm -f flowm *.o

.PHONY: all install uninstall clean
