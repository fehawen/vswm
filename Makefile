LIBS = -lX11
CFLAGS += -std=c99 -Wall -Wextra -pedantic -Os
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
CC ?= gcc

all: flowm

flowm: flowm.o
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 flowm $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/flowm

clean:
	rm -f flowm *.o

.PHONY: all install uninstall clean
