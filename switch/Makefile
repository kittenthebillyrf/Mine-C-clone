# Nintendo Switch build for Mine C++ Clone
# Uses the community raylib-nx backend and the devkitPro/libnx toolchain.

ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO is not set; use the devkitpro/devkita64 build environment)
endif
ifeq ($(strip $(RAYLIB_NX)),)
$(error RAYLIB_NX is not set; point it at the raylib-nx checkout)
endif

TOPDIR := $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

TARGET := MineCppClone
BUILD := build
SOURCES := ../src ../src/i18n ../src/world ../src/render
DATA :=
INCLUDES := ../src
ROMFS := romfs

ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS := -g -Wall -Wextra -O2 -ffunction-sections $(ARCH) $(DEFINES)
CFLAGS += $(INCLUDE) -D__SWITCH__
CXXFLAGS := $(CFLAGS) -std=gnu++20
ASFLAGS := -g $(ARCH)
LDFLAGS = -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS := -lraylib -lEGL -lGLESv2 -lglapi -ldrm_nouveau -lnx -lm
LIBDIRS := $(PORTLIBS) $(LIBNX)

APP_TITLE := Mine C++ Clone
APP_AUTHOR := Mine C++ Clone
APP_VERSION := 0.5.0
NO_ICON := 1

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) $(CURDIR)
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SRC := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN := $(addsuffix .h,$(subst .,_,$(BINFILES)))
export INCLUDE := -I$(RAYLIB_NX)/src -I$(RAYLIB_NX)/src/switch-nx $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := -L$(RAYLIB_NX)/src -L$(RAYLIB_NX)/src/switch-nx $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export LD := $(CXX)
export APP_TITLE APP_AUTHOR APP_VERSION
export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
.PHONY: all clean prepare-romfs
all: prepare-romfs $(BUILD)
prepare-romfs:
	@mkdir -p $(ROMFS)/assets
	@test -s ../assets/blocks_atlas_1_12.png
	@test -s ../assets/block_texture_map.json
	@cp -f ../assets/blocks_atlas_1_12.png $(ROMFS)/assets/
	@cp -f ../assets/block_texture_map.json $(ROMFS)/assets/
$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
clean:
	@rm -rf $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf
else
DEPENDS := $(OFILES:.o=.d)
.PHONY: all
all: $(OUTPUT).nro
$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf: $(OFILES)
$(OFILES_SRC): $(HFILES_BIN)
-include $(DEPENDS)
endif
