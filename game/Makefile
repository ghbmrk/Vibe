GBDK = /opt/gbdk
LCC = $(GBDK)/bin/lcc
ROM_NAME = creature_quest

SRCDIR = src
OBJDIR = obj

SRCS = $(SRCDIR)/main.c \
       $(SRCDIR)/rng.c \
       $(SRCDIR)/gfx_data.c \
       $(SRCDIR)/creature.c \
       $(SRCDIR)/ui.c \
       $(SRCDIR)/battle.c \
       $(SRCDIR)/world.c \
       $(SRCDIR)/save.c

OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

CFLAGS = -Wa-l -Wl-m -Wl-j -Wf--opt-code-speed
LFLAGS = -Wl-yt0x1B -Wl-yo4 -Wl-ya4 -Wm-yn"CREATUREQUEST"

# MBC5+RAM+BATTERY, 4 ROM banks, 4 SRAM banks

all: $(ROM_NAME).gbc

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(LCC) $(CFLAGS) -c -o $@ $<

$(ROM_NAME).gbc: $(OBJS)
	$(LCC) $(LFLAGS) -o $@ $^

clean:
	rm -rf $(OBJDIR) *.gbc *.ihx *.map *.noi *.sym *.lk *.cdb

.PHONY: all clean
