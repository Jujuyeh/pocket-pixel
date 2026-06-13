ARDUINO_CLI ?= arduino-cli
FQBN ?= arduino:avr:leonardo
SKETCH_DIR := $(CURDIR)
BUILD ?= stable
BUILD_DIR ?= $(SKETCH_DIR)/build/$(BUILD)
DIST_DIR ?= $(SKETCH_DIR)/dist
PROFILE ?= profiles/pixel.json
PROFILE_HEADER ?= src/generated/ActivePersonalityProfile.h
PROFILE_TOOL ?= skills/pet-personality-profile/scripts/profile_tool.py
FX_BANNER ?= $(shell $(PROFILE_TOOL) asset $(PROFILE) banner --default assets/fx/banner.png 2>/dev/null || printf '%s' assets/fx/banner.png)
ARDUINO_DATA_DIR ?= $(SKETCH_DIR)/.arduino
PORT ?=
TARGET ?= fx
OPEN ?= xdg-open
RETROARCH ?= retroarch
ARDENS_LIBRETRO_CORE ?=
RETROARCH_CONFIG ?= $(SKETCH_DIR)/config/retroarch/arduboy-clean.cfg
FX_CART_DIR ?= $(DIST_DIR)/fx-cart
FX_CATEGORY ?= 99-Development
FX_GAME ?= 01-Pocket-Pixel

ELF := $(BUILD_DIR)/pocket-pixel.ino.elf
HEX := $(BUILD_DIR)/pocket-pixel.ino.hex
AVR_SIZE ?= avr-size
AVR_NM ?= avr-nm
PET_STUDIO ?= $(SKETCH_DIR)/tools/pet-studio/index.html
PET_STUDIO_PORT ?= 8123
PET_STUDIO_SERVER ?= $(SKETCH_DIR)/tools/pet-studio/server.py
SPRITE_TOOL ?= $(SKETCH_DIR)/tools/pet-studio/sprite_tool.py
SPRITES_JSON ?= $(SKETCH_DIR)/build/pet-studio/assets.json
AUDIO_TOOL ?= $(SKETCH_DIR)/tools/pet-studio/audio_tool.py
AUDIO_JSON ?= $(SKETCH_DIR)/build/pet-studio/audio.json
WEB_SITE_SCRIPT ?= $(SKETCH_DIR)/tools/prepare-web-site.sh

ifeq ($(BUILD),debug)
ARDUINO_BUILD_FLAGS := --build-property compiler.cpp.extra_flags="-DPOCKET_PIXEL_DEBUG=1"
else ifeq ($(BUILD),stable)
ARDUINO_BUILD_FLAGS :=
else
$(error BUILD must be stable or debug)
endif

export ARDUINO_DIRECTORIES_DATA := $(ARDUINO_DATA_DIR)
export ARDUINO_DIRECTORIES_USER := $(SKETCH_DIR)/.arduino-sketchbook

.PHONY: all setup compile compile-debug profile-header upload upload-sketch clean hex size size-debug symbols symbols-debug pet-studio asset-studio sprite-studio sprites-json audio-json check env sim cloud libretro libretro-debug fx-entry web-site

all: compile

env:
	@$(ARDUINO_CLI) version
	@$(ARDUINO_CLI) core list
	@$(ARDUINO_CLI) lib list

setup:
	$(ARDUINO_CLI) core update-index
	$(ARDUINO_CLI) core install arduino:avr
	$(ARDUINO_CLI) lib install Arduboy2 Arduboy-TinyFont ArduboyTones

compile:
	$(ARDUINO_CLI) compile --fqbn $(FQBN) $(ARDUINO_BUILD_FLAGS) --build-path $(BUILD_DIR) $(SKETCH_DIR)

compile-debug:
	$(MAKE) compile BUILD=debug

profile-header:
	$(PROFILE_TOOL) header $(PROFILE) --output $(PROFILE_HEADER)

upload:
ifeq ($(TARGET),fx)
	$(error TARGET=fx protects your Arduboy FX catalog. Use make fx-entry, then merge that entry into a backed-up flashcart image)
endif
	$(MAKE) upload-sketch TARGET=$(TARGET) PORT=$(PORT) CONFIRM_OVERWRITE=$(CONFIRM_OVERWRITE)

upload-sketch:
ifneq ($(CONFIRM_OVERWRITE),1)
	$(error Refusing to overwrite the main sketch. Re-run with CONFIRM_OVERWRITE=1 after confirming this is not an FX catalog update)
endif
ifndef PORT
	$(error Set PORT=/dev/ttyACM0 or another Arduboy serial device)
endif
	$(ARDUINO_CLI) upload --fqbn $(FQBN) --port $(PORT) $(SKETCH_DIR)

hex: compile
	@printf '%s\n' "$(HEX)"

size: compile
	$(AVR_SIZE) -C --mcu=atmega32u4 "$(ELF)"
	$(AVR_SIZE) -A "$(ELF)"

size-debug:
	$(MAKE) size BUILD=debug

symbols: compile
	$(AVR_NM) --print-size --size-sort --radix=d "$(ELF)"

symbols-debug:
	$(MAKE) symbols BUILD=debug

pet-studio: sprites-json audio-json
	@printf '%s\n' "Opening Pet Studio with project assets:"
	@printf '%s\n' "http://127.0.0.1:$(PET_STUDIO_PORT)/tools/pet-studio/"
	$(PET_STUDIO_SERVER) --port "$(PET_STUDIO_PORT)" --open

asset-studio: pet-studio
sprite-studio: asset-studio

sprites-json:
	$(SPRITE_TOOL) extract "$(SKETCH_DIR)/src/Assets.cpp" --output "$(SPRITES_JSON)"
	@printf '%s\n' "$(SPRITES_JSON)"

audio-json:
	$(AUDIO_TOOL) extract "$(SKETCH_DIR)/src/Assets.cpp" --output "$(AUDIO_JSON)"
	@printf '%s\n' "$(AUDIO_JSON)"

sim: compile
	@printf '%s\n' "Open this HEX in Arduboy Cloud, Ardens, or another Arduboy emulator:"
	@printf '%s\n' "$(HEX)"

cloud: compile
	@printf '%s\n' "Opening Arduboy Cloud. Load this HEX in the emulator:"
	@printf '%s\n' "$(HEX)"
	$(OPEN) "https://cloud.arduboy.com/"

libretro: compile
ifndef ARDENS_LIBRETRO_CORE
	$(error Enter nix develop first, or set ARDENS_LIBRETRO_CORE=/path/to/ardens_libretro.so)
endif
	$(RETROARCH) --appendconfig="$(RETROARCH_CONFIG)" --set-shader="" -L "$(ARDENS_LIBRETRO_CORE)" "$(HEX)"

libretro-debug:
	$(MAKE) libretro BUILD=debug

fx-entry: compile
	install -D "$(HEX)" "$(FX_CART_DIR)/$(FX_CATEGORY)/$(FX_GAME).hex"
	install -D "$(SKETCH_DIR)/$(FX_BANNER)" "$(FX_CART_DIR)/$(FX_CATEGORY)/$(FX_GAME).png"
	@printf '%s\n' "FX catalog entry prepared:"
	@printf '%s\n' "$(FX_CART_DIR)/$(FX_CATEGORY)/$(FX_GAME).hex"
	@printf '%s\n' "$(FX_CART_DIR)/$(FX_CATEGORY)/$(FX_GAME).png"
	@printf '%s\n' "Banner: $(FX_BANNER)"
	@printf '%s\n' "Merge this into a backup of your existing FX cart before writing."

web-site: compile
	$(WEB_SITE_SCRIPT) "$(HEX)"

check: compile

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
