CFLAGS+= -Wall
LDADD+= -lxcb
LDFLAGS=
EXEC=bluckywm

PREFIX?= /usr
BINDIR?= $(PREFIX)/bin

CC=gcc

all: $(EXEC)

bluckywm: bluckywm.o
	$(CC) $(LDFLAGS) -Os -o $@ $+ $(LDADD)

install: all
	install -Dm 755 bluckywm $(DESTDIR)$(BINDIR)/bluckywm

clean:
	rm -fv bluckywm *.o
