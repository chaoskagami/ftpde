CFILES  := $(wildcard source/*.c)
OFILES  := $(patsubst source/%,build.posix/%,$(CFILES:.c=.o))

PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin
SYSCONFDIR ?= /etc

CFLAGS  := -g -Wall -Iinclude -U_3DS -DSTATUS_STRING="\"ftpde $(VERSION)\"" -DSYSCONFDIR="\"$(SYSCONFDIR)\""
LDFLAGS :=
LIBS    := -lconfig

INSTALL := install -v

OUTPUT  := ftpde

.PHONY: all clean

all: build.posix $(OUTPUT)

build.posix:
	@mkdir build.posix/

$(OUTPUT): $(OFILES)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(OFILES): build.posix/%.o : source/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	$(RM) -r build.posix/ $(OUTPUT)

install: $(OUTPUT)
	[ ! -f $(SYSCONFDIR)/ftpde.conf ] && $(INSTALL) -m644 extra/ftpde.conf $(SYSCONFDIR)/ftpde.conf
	$(INSTALL) -m755 -d $(PREFIX)
	$(INSTALL) -m755 -d $(BINDIR)
	$(INSTALL) -m755 $(OUTPUT) $(BINDIR)/$(OUTPUT)
