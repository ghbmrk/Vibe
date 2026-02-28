/* shop.c - Vendor / gear shop */
#include "shop.h"
#include "character.h"
#include "creature.h"
#include "ui.h"

/* ── Gear catalog ──────────────────────────────────────────── */
/*  Items 0-5: character gear, items 6-11: creature gear
    Bonus scales by zone (added when purchasing).
    Base bonus values are per-tier multipliers. */
const GearDef gear_catalog[MAX_GEAR_ITEMS] = {
    /* Character weapons (CHA bonus) */
    {"POWER SEAL",  60, 1, GEAR_WEAPON,    0},
    {"WAR BANNER",  120, 2, GEAR_WEAPON,   0},
    /* Character armor (WIS bonus) */
    {"RELICPLATE", 60, 1, GEAR_ARMOR,     0},
    {"AEGIS WARD",  120, 2, GEAR_ARMOR,    0},
    /* Character accessories (LCK bonus) */
    {"LUCKY COIN",  60, 1, GEAR_ACCESSORY, 0},
    {"EMP. TAROT", 120, 2, GEAR_ACCESSORY,0},

    /* Creature weapons (ATK bonus) */
    {"FANG BLADE",  80, 2, GEAR_WEAPON,    1},
    {"CLAW GAUNT",  160, 4, GEAR_WEAPON,   1},
    /* Creature armor (DEF bonus) */
    {"IRON SHELL",  80, 2, GEAR_ARMOR,     1},
    {"BONE PLATE",  160, 4, GEAR_ARMOR,    1},
    /* Creature accessories (SPD bonus) */
    {"SWIFTCHARM", 80, 2, GEAR_ACCESSORY, 1},
    {"WIND RUNE",   160, 4, GEAR_ACCESSORY,1},
};

/* ── Effective cost with WIS discount ──────────────────────── */
uint16_t shop_effective_cost(uint8_t gear_id) {
    uint16_t base;
    uint16_t mult;
    if (gear_id >= MAX_GEAR_ITEMS) return 9999;
    base = gear_catalog[gear_id].cost;
    mult = character_cost_mult(&game.player);
    return (base * mult) / 100;
}

/* ── Shop UI ───────────────────────────────────────────────── */
void shop_run(uint8_t zone_num) {
    uint8_t sel = 0;
    uint8_t pressed;
    uint8_t i;
    uint8_t page = 0; /* 0 = char gear, 1 = creature gear */
    uint8_t item_offset;
    uint16_t cost;
    const GearDef *item;

    (void)zone_num;

    ui_clear_screen();
    ui_draw_box(0, 0, 20, 18);

    for (;;) {
        /* Header */
        ui_print(1, 1, "IMPERIAL ARMORY");
        ui_print(1, 2, "GOLD: ");
        ui_print_num(7, 2, game.player.gold);

        /* Tab labels */
        if (page == 0) {
            ui_print(1, 3, ">MARINE GEAR");
            ui_print(1, 4, " CREATURE GEAR");
        } else {
            ui_print(1, 3, " MARINE GEAR");
            ui_print(1, 4, ">CREATURE GEAR");
        }

        item_offset = page * 6;

        /* List items */
        for (i = 0; i < 6; i++) {
            uint8_t y = 6 + i * 2;
            item = &gear_catalog[item_offset + i];
            cost = shop_effective_cost(item_offset + i);

            ui_clear_rect(1, y, 18, 2);
            ui_print(3, y, item->name);
            ui_print(14, y, "$");
            ui_print_num(15, y, cost);

            /* Show bonus */
            ui_print(3, y + 1, "+");
            ui_print_num(4, y + 1, (uint16_t)item->bonus);
            if (item->for_creature) {
                switch (item->slot) {
                    case GEAR_WEAPON:    ui_print(7, y+1, "ATK"); break;
                    case GEAR_ARMOR:     ui_print(7, y+1, "DEF"); break;
                    case GEAR_ACCESSORY: ui_print(7, y+1, "SPD"); break;
                }
            } else {
                switch (item->slot) {
                    case GEAR_WEAPON:    ui_print(7, y+1, "CHA"); break;
                    case GEAR_ARMOR:     ui_print(7, y+1, "WIS"); break;
                    case GEAR_ACCESSORY: ui_print(7, y+1, "LCK"); break;
                }
            }

            /* Cursor */
            set_bkg_tile_xy(1, y,
                            (i == sel) ? TILE_CURSOR : TILE_FONT_BASE);
        }

        wait_vbl_done();
        pressed = ui_poll_keys();

        if (pressed & J_UP) {
            if (sel > 0) sel--;
        }
        if (pressed & J_DOWN) {
            if (sel < 5) sel++;
        }
        if (pressed & J_LEFT) {
            page = 0; sel = 0;
        }
        if (pressed & J_RIGHT) {
            page = 1; sel = 0;
        }
        if (pressed & J_B) {
            /* Exit shop */
            return;
        }
        if (pressed & J_A) {
            uint8_t gear_id = item_offset + sel;
            item = &gear_catalog[gear_id];
            cost = shop_effective_cost(gear_id);

            if (game.player.gold < cost) {
                ui_message("NOT ENOUGH", "GOLD!");
                continue;
            }

            /* Purchase */
            game.player.gold -= cost;

            if (item->for_creature) {
                game.creature.gear[item->slot] = gear_id;
                creature_calc_stats(&game.creature);
                ui_message("EQUIPPED ON", species_names[game.creature.species]);
            } else {
                game.player.gear[item->slot] = gear_id;
                ui_message("EQUIPPED!", "");
            }

            /* Redraw gold */
            ui_clear_rect(7, 2, 8, 1);
            ui_print_num(7, 2, game.player.gold);
        }
    }
}
