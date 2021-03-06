#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#
# NO_SMDH: if set to anything, no SMDH file is generated.
# APP_TITLE is the name of the app stored in the SMDH file (Optional)
# APP_DESCRIPTION is the description of the app stored in the SMDH file (Optional)
# APP_AUTHOR is the author of the app stored in the SMDH file (Optional)
# ICON is the filename of the icon (.png), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.png
#     - icon.png
#     - <libctru folder>/default_icon.png
#---------------------------------------------------------------------------------
TARGET   := ftpde-$(VERSION)
BUILD    := build
SOURCES  := source
DATA     := gfx
INCLUDES := include
ROMFS    :=

APP_TITLE       := ftpde $(VERSION)
APP_DESCRIPTION := ftpd + ftp-gmx + hax $(VERSION)
APP_AUTHOR      := mtheall/Vorpal Blade/chaoskagami
ICON            := app_icon.png
HOST            ?= 192.168.1.2

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH     := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS   := -g -Wall -O3 -mword-relocations \
             -fomit-frame-pointer -ffunction-sections \
            $(ARCH) \
            -DSTATUS_STRING="\"ftpde $(VERSION)\""

CFLAGS   +=  $(INCLUDE) -DARM11 -D_3DS

CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS  := -g $(ARCH)
LDFLAGS   = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(TARGET).map

LIBS     := -lsfil -lpng -ljpeg -lz -lsf2d -lctru -lm -lconfig

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS  := $(CTRULIB) $(PORTLIBS)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT  :=  $(CURDIR)/$(TARGET)
export TOPDIR  :=  $(CURDIR)

export VPATH   :=  $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                   $(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR :=  $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
  export LD := $(CC)
else
  export LD := $(CXX)
endif
#---------------------------------------------------------------------------------

export OFILES   := $(addsuffix .o,$(BINFILES)) \
                   $(CPPFILES:.cpp=.o) \
                   $(CFILES:.c=.o) \
                   $(SFILES:.s=.o)

export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                   -I$(CURDIR)/$(BUILD)

export LIBPATHS :=  $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.png)
	ifneq (,$(findstring $(TARGET).png,$(icons)))
		export APP_ICON := $(TOPDIR)/gfx/$(TARGET).png
	else
		ifneq (,$(findstring icon.png,$(icons)))
			export APP_ICON := $(TOPDIR)/gfx/icon.png
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/gfx/$(ICON)
endif

ifeq ($(strip $(NO_SMDH)),)
	export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
endif

ifneq ($(ROMFS),)
	export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	[ -d $@ ] || mkdir -p $@
	make --no-print-directory -C $(BUILD) -f $(CURDIR)/3ds.mk

#---------------------------------------------------------------------------------
clean:
	rm -fr $(BUILD) $(TARGET).3dsx $(OUTPUT).smdh $(TARGET)_3ds.elf $(TARGET).cia $(TARGET).3ds


#---------------------------------------------------------------------------------
else

DEPENDS := $(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
ifeq ($(strip $(NO_SMDH)),)
.PHONY: all
all	:	$(OUTPUT).3dsx $(OUTPUT).smdh
$(OUTPUT).smdh : $(TOPDIR)/Makefile
$(OUTPUT).3dsx: $(OUTPUT).smdh
endif

$(OUTPUT).3dsx: $(OUTPUT)_3ds.elf $(OUTPUT).cia $(OUTPUT).3ds ban.bnr ico.icn

$(OUTPUT)_3ds.elf:  $(OFILES)

$(OUTPUT).cia    :     $(OUTPUT)_3ds.elf
	cp $(OUTPUT)_3ds.elf $(TARGET)_stripped_3ds.elf
	$(PREFIX)strip $(TARGET)_stripped_3ds.elf
	bannertool makebanner -i $(TOPDIR)/gfx/app_banner.png -a $(TOPDIR)/extra/default.wav -o ban.bnr
	bannertool makesmdh -s "FTP" -l "ftpde" -p "chaoskagami" -i $(TOPDIR)/gfx/app_icon.png -o ico.icn
	makerom -f cia -o $(OUTPUT).cia -DAPP_ENCRYPTED=false -rsf $(TOPDIR)/extra/info.rsf -target t -exefslogo -elf $(TARGET)_stripped_3ds.elf -icon $(TOPDIR)/build/ico.icn -banner $(TOPDIR)/build/ban.bnr

$(OUTPUT).3ds    :     $(OUTPUT)_3ds.elf
	cp $(OUTPUT)_3ds.elf $(TARGET)_stripped_3ds.elf
	$(PREFIX)strip $(TARGET)_stripped_3ds.elf
	makerom -f cci -o $(OUTPUT).3ds -DAPP_ENCRYPTED=true -rsf $(TOPDIR)/extra/info.rsf -target t -exefslogo -elf $(TARGET)_stripped_3ds.elf -icon $(TOPDIR)/build/ico.icn -banner $(TOPDIR)/build/ban.bnr

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o: %.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
%.png.o	:	%.png
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------

install: $(OUTPUT).cia
	python extra/FalconPunch.py $(OUTPUT).cia $(HOST)
