# Nintendo Switch homebrew build (devkitPro + libnx + raylib-nx)
.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO is not set. Install devkitPro Switch development tools.)
endif
ifeq ($(strip $(RAYLIB_NX)),)
$(error RAYLIB_NX is not set to the raylib-nx source directory.)
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

TARGET := MineCppClone
BUILD := build-switch
SOURCES := src src/i18n src/world src/world/generation src/render
INCLUDES := src
APP_TITLE := Mine C++ Clone
APP_AUTHOR := Mine C++ Clone Project
APP_VERSION := 0.4.0
ICON := $(TOPDIR)/icon.jpg

ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS := -O2 -Wall -Wextra -ffunction-sections -fdata-sections $(ARCH) $(DEFINES)
CFLAGS += $(INCLUDE) -D__SWITCH__
CXXFLAGS := $(CFLAGS) -std=gnu++20
LDFLAGS = -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*).map -Wl,--gc-sections
LIBS := -lraylib -lEGL -lGLESv2 -lglapi -ldrm_nouveau -lnx -lm
LIBDIRS := $(PORTLIBS) $(LIBNX) $(RAYLIB_NX)/src

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
export OFILES_SRC := $(CPPFILES:.cpp=.o)
export OFILES := $(OFILES_SRC)
export LD := $(CXX)
export INCLUDE := -I$(RAYLIB_NX)/src -I$(RAYLIB_NX)/src/switch-nx $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := -L$(RAYLIB_NX)/src -L$(RAYLIB_NX)/src/switch-nx $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all clean
all: $(BUILD)
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
clean:
	@rm -rf $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf
else
DEPENDS := $(OFILES:.o=.d)
.PHONY: all
all: $(OUTPUT).nro
$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf: $(OFILES)
-include $(DEPENDS)
endif
