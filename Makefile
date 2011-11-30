CFLAGS+= -Wall
LDADD+= -lX11
LDFLAGS=
EXEC=woodywm

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin

CC=gcc

all: $(EXEC)

woodywm: woodywm.o
	$(CC) $(LDFLAGS) -s -Os -o $@ $+ $(LDADD)

install: all
	install -Dm 755 woodywm $(DESTDIR)$(BINDIR)/woodywm

clean:
	rm -fv woodywm *.o

