.PHONY: all clean posix

export VERSION := $(shell cat VERSION)-$(shell git rev-parse HEAD | head -c8)

all:
	@echo "Run make with one of the following platforms: posix, 3ds"

3ds:
	$(MAKE) -f 3ds.mk

fbi-install:
	$(MAKE) -f 3ds.mk install

install:
	$(MAKE) -f posix.mk install

posix:
	$(MAKE) -f posix.mk

clean:
	$(MAKE) -f 3ds.mk   clean
	$(MAKE) -f posix.mk clean

release: 3ds posix
	mkdir $(VERSION)-release
	cp ftpde ftpde-$(VERSION).3dsx ftpde-$(VERSION).3ds ftpde-$(VERSION).cia ftpde-$(VERSION).smdh extra/ftpde.conf $(VERSION)-release
	zip -r9 $(VERSION)-release.zip $(VERSION)-release
