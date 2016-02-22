.PHONY: all clean linux

export VERSION := 2.3

all:
	@echo "Run make with one of the following targets: linux, 3ds"

3ds:
	@$(MAKE) -f 3ds.mk

linux:
	@$(MAKE) -f linux.mk

clean:
	@$(MAKE) -f 3ds.mk   clean
	@$(MAKE) -f linux.mk clean
