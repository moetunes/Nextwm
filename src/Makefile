CFLAGS+= -Wall
LDADD+= -lX11 
LDFLAGS=
EXEC=DMiniWM

PREFIX?= /usr
BINDIR?= $(PREFIX)/bin

CC=gcc

all: $(EXEC)

DMiniWM: DMiniWM.o
	$(CC) $(LDFLAGS) -Os -o $@ $+ $(LDADD)

install: all
	install -Dm 755 DMiniWM $(DESTDIR)$(BINDIR)/DMiniWM

clean:
	rm -fv DMiniWM *.o
