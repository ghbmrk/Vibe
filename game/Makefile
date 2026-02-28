# ============================================================
# Creature Collector - Game Boy Color game for ModRetro Chromatic
# Built with GBDK-2020 (https://github.com/gbdk-2020/gbdk-2020)
# ============================================================

# GBDK installation path - override with: make GBDK=/your/path
GBDK    ?= /opt/gbdk-2020
CC       = $(GBDK)/bin/lcc

# Output ROM
TARGET   = CreatureCollector.gbc

# Compiler / linker flags
#   -msm83:gbc        Target Game Boy Color
#   -Wl-yt0x1B        MBC5 + RAM + Battery (save support)
#   -Wl-ya1           1 SRAM bank (8 KB, plenty for save data)
#   -Wa-l             Generate assembler listing
#   -Wl-m             Generate linker map file
#   -Wf--max-allocs-per-node50000  Better SDCC optimisation
CFLAGS   = -msm83:gbc \
           -Wl-yt0x1B \
           -Wl-ya1 \
           -Wa-l -Wl-m \
           -Wf--max-allocs-per-node50000 \
           -Isrc

# Source files (order matters for GBDK linking)
SRCS = src/main.c \
       src/rng.c \
       src/creature.c \
       src/skilltree.c \
       src/battle.c \
       src/world.c \
       src/ui.c \
       src/save.c \
       src/gfx_data.c

# ---- Targets ------------------------------------------------

.PHONY: all clean serve test watch

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) *.map *.noi *.lst *.sym *.adb *.ihx *.lk
	rm -f src/*.asm src/*.lst src/*.sym src/*.adb

# Start dev server for mobile testing (phone on same WiFi)
serve:
	./serve.sh

# Build + serve in one command
test: all serve

# Auto-rebuild on source changes (requires fswatch or inotifywait)
watch:
	@echo "Watching src/ for changes... (Ctrl+C to stop)"
	@if command -v fswatch >/dev/null 2>&1; then \
		fswatch -o src/ | while read; do echo "--- Rebuilding ---"; $(MAKE) all; done; \
	elif command -v inotifywait >/dev/null 2>&1; then \
		while inotifywait -qre modify src/; do echo "--- Rebuilding ---"; $(MAKE) all; done; \
	else \
		echo "Install fswatch (macOS: brew install fswatch) or inotifywait (Linux: apt install inotify-tools)"; \
		exit 1; \
	fi
