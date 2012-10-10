CFLAGS+= -g -std=c99 -pedantic -Wall
LDADD+= -lX11 -lXinerama
LDFLAGS=
EXEC=snapwm

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin

CC=gcc

all: $(EXEC)

snapwm: snapwm.o
	$(CC) $(LDFLAGS) -s -O2 -ffast-math -fno-unit-at-a-time -o $@ $+ $(LDADD)

install: all
	install -Dm 755 snapwm $(DESTDIR)$(BINDIR)/snapwm

clean:
	rm -fv snapwm *.o

