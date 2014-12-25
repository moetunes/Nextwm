CFLAGS+= -g -std=c99 -pedantic -Wall -O2 -pipe -fstack-protector --param=ssp-buffer-size=4 -D_FORTIFY_SOURCE=2
LDADD+= -lX11 -lXinerama -lXrandr
LDFLAGS= -Wl,-O1,--sort-common,--as-needed,-z,relro
EXEC=snapwm

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin

CC=gcc

all: $(EXEC)

snapwm: snapwm.o
	$(CC) $(LDFLAGS) -s -ffast-math -fno-unit-at-a-time -o $@ $+ $(LDADD)

install: all
	install -Dm 755 snapwm $(DESTDIR)$(BINDIR)/snapwm

clean:
	rm -fv snapwm *.o

