CFILES  := $(wildcard source/*.c)
OFILES  := $(patsubst source/%,build.linux/%,$(CFILES:.c=.o))

CFLAGS  := -g -Wall -Iinclude -U_3DS -DSTATUS_STRING="\"ftpde $(VERSION)\""
LDFLAGS :=

OUTPUT  := ftpde

.PHONY: all clean

all: build.linux $(OUTPUT)

build.linux:
	@mkdir build.linux/

$(OUTPUT): $(OFILES)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OFILES): build.linux/%.o : source/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	$(RM) -r build.linux/ $(OUTPUT)
