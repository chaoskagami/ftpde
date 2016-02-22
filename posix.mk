CFILES  := $(wildcard source/*.c)
OFILES  := $(patsubst source/%,build.posix/%,$(CFILES:.c=.o))

CFLAGS  := -g -Wall -Iinclude -U_3DS -DSTATUS_STRING="\"ftpde $(VERSION)\""
LDFLAGS :=
LIBS    := -lconfig

PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin
SBINDIR  ?= $(PREFIX)/sbin

INSTALL := install -m755 -v

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
	$(INSTALL) -d $(PREFIX)
	$(INSTALL) -d $(BINDIR)
	$(INSTALL) $(OUTPUT) $(BINDIR)/$(OUTPUT)
