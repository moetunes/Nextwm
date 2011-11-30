CFLAGS+= -Wall
LDADD+= -lX11
LDFLAGS=
EXEC=snapwm

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin

CC=gcc

all: $(EXEC)

snapwm: snapwm.o
	$(CC) $(LDFLAGS) -s -Os -o $@ $+ $(LDADD)

install: all
	install -Dm 755 snapwm $(DESTDIR)$(BINDIR)/snapwm

clean:
	rm -fv snapwm *.o

