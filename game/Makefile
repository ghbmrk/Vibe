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

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) *.map *.noi *.lst *.sym *.adb *.ihx *.lk
	rm -f src/*.asm src/*.lst src/*.sym src/*.adb
