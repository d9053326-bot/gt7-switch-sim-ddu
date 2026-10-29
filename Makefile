#---------------------------------------------------------------------------------
# Dual-Mode Makefile:
# 1. Native Nintendo Switch Homebrew (.nro) when DEVKITPRO is set or inside docker
# 2. Local macOS / Linux Desktop Simulator & Live PS5 Receiver via `make host`
#---------------------------------------------------------------------------------

TARGET_HOST := gt7_switch_hud_sim
HOST_CC     := clang
HOST_SRCS   := source/main.c source/hud_renderer.c source/gt7_telemetry.c source/salsa20.c

ifneq ($(strip $(DEVKITPRO)),)
# ================= NINTENDO SWITCH LIBNX (.NRO) BUILD =================
include $(DEVKITPRO)/libnx/switch_rules

TARGET      := gt7_switch_hud
BUILD       := build_switch
SOURCES     := source
INCLUDES    := source

APP_TITLE   := GT7 Telemetry HUD
APP_AUTHOR  := Switch SimRacing Lab
APP_VERSION := _aw_1.10.29.0839

ARCH        := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS      := -g -Wall -O2 -ffunction-sections $(ARCH) $(DEFINES)
CFLAGS      += $(INCLUDE) -D__SWITCH__ `$(PREFIX)pkg-config --cflags sdl2`
CXXFLAGS    := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS     := -g $(ARCH)
LDFLAGS     := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS        := `$(PREFIX)pkg-config --libs sdl2` -lEGL -lglapi -ldrm_nouveau -lnx -lm

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT   := $(CURDIR)/$(TARGET)
export TOPDIR   := $(CURDIR)
export VPATH    := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR  := $(CURDIR)/$(BUILD)
CFILES          := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export OFILES   := $(CFILES:.c=.o)
export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                   -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean all host switch-docker

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf $(TARGET_HOST)

else
DEPENDS := $(OFILES:.o=.d)
$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf: $(OFILES)
-include $(DEPENDS)
endif

else
# ================= HOST (macOS / Linux) & DOCKER SWITCH BUILD =================
SDL_CFLAGS  := $(shell sdl2-config --cflags)
SDL_LIBS    := $(shell sdl2-config --libs)
HOST_CFLAGS := -Wall -Wextra -O2 $(SDL_CFLAGS) -I./source

.PHONY: all host run switch-docker clean

all: host

host: $(TARGET_HOST)
	@echo "✅ Built desktop simulator & live PS5 receiver: ./$(TARGET_HOST)"

$(TARGET_HOST): $(HOST_SRCS)
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_SRCS) -o $(TARGET_HOST) $(SDL_LIBS) -lm

run: $(TARGET_HOST)
	./$(TARGET_HOST)

switch-docker:
	@echo "Building Nintendo Switch .nro via devkitpro/devkita64 Docker container..."
	docker run --rm --platform linux/amd64 -v "$(CURDIR):/app" -w /app devkitpro/devkita64:latest \
		bash -c "dkp-pacman -Syu --noconfirm switch-sdl2 switch-mesa switch-glad >/dev/null 2>&1; make -f Makefile.switch"
	@echo "Done: gt7_switch_hud.nro"

clean:
	rm -rf $(TARGET_HOST) build_switch *.nro *.nacp *.elf
endif
