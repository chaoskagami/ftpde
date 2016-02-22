.PHONY: all clean posix

export VERSION := 1.0

all:
	@echo "Run make with one of the following platforms: posix, 3ds"

3ds:
	@$(MAKE) -f 3ds.mk

fbi-install:
	@$(MAKE) -f 3ds.mk install

install:
	@$(MAKE) -f posix.mk install

posix:
	@$(MAKE) -f posix.mk

clean:
	@$(MAKE) -f 3ds.mk   clean
	@$(MAKE) -f posix.mk clean
