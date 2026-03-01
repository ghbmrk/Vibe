#include "world.h"
#include "creature.h"
#include "ui.h"
#include "rng.h"
#include "gfx_data.h"
#include "battle.h"
#include <gb/gb.h>
#include <gb/cgb.h>

/* Map data for current zone */
static uint8_t map_tiles[MAP_W * MAP_H];

/* Terrain tile indices in terrain tileset */
#define T_GRASS1  0
#define T_GRASS2  1
#define T_TALL    2
#define T_PATH    3
#define T_WATER   4
#define T_WATER2  5
#define T_TREETOP 6
#define T_TREEBOTTOM 7
#define T_ROCK    8
#define T_HOUSE_TL 9
#define T_HOUSE_TR 10
#define T_HOUSE_BL 11
#define T_HOUSE_BR 12
#define T_DOOR    13
#define T_SIGN    14
#define T_BRIDGE  15

/* Is this tile walkable? */
static uint8_t is_walkable(uint8_t tile) {
    switch (tile) {
        case T_GRASS1:
        case T_GRASS2:
        case T_TALL:
        case T_PATH:
        case T_BRIDGE:
        case T_DOOR:
        case T_SIGN:
            return 1;
        default:
            return 0;
    }
}

/* =========================================================
   PROCEDURAL ZONE GENERATION
   Creates a unique map for each zone using seed
   ========================================================= */
void world_generate_zone(void) {
    uint8_t x, y;
    uint8_t zone = save.current_zone;
    uint16_t i;

    /* Seed RNG with zone number for consistent generation */
    /* (we re-seed the rng so zones are always the same layout) */

    /* Fill with base grass */
    for (i = 0; i < MAP_W * MAP_H; i++) {
        map_tiles[i] = T_GRASS1;
    }

    /* Border with trees */
    for (x = 0; x < MAP_W; x++) {
        map_tiles[x] = T_TREETOP;
        map_tiles[MAP_W + x] = T_TREEBOTTOM;
        map_tiles[(MAP_H - 2) * MAP_W + x] = T_TREETOP;
        map_tiles[(MAP_H - 1) * MAP_W + x] = T_TREEBOTTOM;
    }
    for (y = 0; y < MAP_H; y++) {
        map_tiles[y * MAP_W] = T_ROCK;
        map_tiles[y * MAP_W + MAP_W - 1] = T_ROCK;
    }

    /* Path from left to right through center */
    for (x = 1; x < MAP_W - 1; x++) {
        uint8_t py = 8 + (x % 3 == 0 ? 1 : 0) - (x % 5 == 0 ? 1 : 0);
        map_tiles[py * MAP_W + x] = T_PATH;
        if (py > 2 && py < MAP_H - 2) {
            map_tiles[(py - 1) * MAP_W + x] = T_PATH;
        }
    }

    /* Scatter tall grass (encounter zones) */
    for (i = 0; i < 30 + zone * 5; i++) {
        x = rng_range(2, MAP_W - 3);
        y = rng_range(3, MAP_H - 4);
        if (map_tiles[y * MAP_W + x] == T_GRASS1) {
            map_tiles[y * MAP_W + x] = T_TALL;
        }
    }

    /* Add some flower patches */
    for (i = 0; i < 10; i++) {
        x = rng_range(2, MAP_W - 3);
        y = rng_range(3, MAP_H - 4);
        if (map_tiles[y * MAP_W + x] == T_GRASS1) {
            map_tiles[y * MAP_W + x] = T_GRASS2;
        }
    }

    /* Water feature based on zone */
    if (zone == 1 || zone == 4) { /* Water zones */
        for (y = 5; y < 8; y++) {
            for (x = 3; x < 8; x++) {
                map_tiles[y * MAP_W + x] = ((x + y) & 1) ? T_WATER : T_WATER2;
            }
        }
        /* Bridge across */
        map_tiles[6 * MAP_W + 5] = T_BRIDGE;
        map_tiles[6 * MAP_W + 6] = T_BRIDGE;
    }

    /* Rocks/boulders */
    for (i = 0; i < 5 + zone; i++) {
        x = rng_range(2, MAP_W - 3);
        y = rng_range(3, MAP_H - 4);
        if (map_tiles[y * MAP_W + x] == T_GRASS1) {
            map_tiles[y * MAP_W + x] = T_ROCK;
        }
    }

    /* Trees scattered */
    for (i = 0; i < 4 + zone; i++) {
        x = rng_range(2, MAP_W - 3);
        y = rng_range(4, MAP_H - 4);
        if (map_tiles[y * MAP_W + x] == T_GRASS1 &&
            map_tiles[(y - 1) * MAP_W + x] == T_GRASS1) {
            map_tiles[(y - 1) * MAP_W + x] = T_TREETOP;
            map_tiles[y * MAP_W + x] = T_TREEBOTTOM;
        }
    }

    /* Shop building (if zone has one) */
    if (zone_db[zone].has_shop) {
        map_tiles[3 * MAP_W + 14] = T_HOUSE_TL;
        map_tiles[3 * MAP_W + 15] = T_HOUSE_TR;
        map_tiles[4 * MAP_W + 14] = T_HOUSE_BL;
        map_tiles[4 * MAP_W + 15] = T_HOUSE_BR;
        map_tiles[4 * MAP_W + 14] = T_DOOR; /* door on bottom-left */
    }

    /* Sign at entrance */
    map_tiles[8 * MAP_W + 2] = T_SIGN;

    /* Boss area marker (path section at right side) */
    for (y = 6; y < 11; y++) {
        map_tiles[y * MAP_W + MAP_W - 2] = T_PATH;
    }

    /* Set player start position */
    if (save.player_x == 0 && save.player_y == 0) {
        save.player_x = 3;
        save.player_y = 9;
    }
}

/* =========================================================
   DRAW THE MAP
   ========================================================= */
void world_draw(void) {
    uint8_t x, y;
    uint8_t tile_row[MAP_W];
    uint8_t pal = zone_db[save.current_zone].terrain_palette;

    /* Set BG palette for entire map */
    ui_set_area_palette(0, 0, 20, 18, pal);

    /* Draw tile map */
    for (y = 0; y < MAP_H; y++) {
        for (x = 0; x < MAP_W; x++) {
            tile_row[x] = TILE_TERRAIN_START + map_tiles[y * MAP_W + x];
        }
        set_bkg_tiles(0, y, MAP_W, 1, tile_row);
    }

    /* Draw player sprite */
    move_sprite(SPR_PLAYER,     (save.player_x * 8) + 8, (save.player_y * 8) + 16);
    move_sprite(SPR_PLAYER + 1, (save.player_x * 8) + 16, (save.player_y * 8) + 16);
    move_sprite(SPR_PLAYER + 2, (save.player_x * 8) + 8, (save.player_y * 8) + 24);
    move_sprite(SPR_PLAYER + 3, (save.player_x * 8) + 16, (save.player_y * 8) + 24);

    /* Set player sprite tiles and palette */
    set_sprite_tile(SPR_PLAYER, 0);
    set_sprite_tile(SPR_PLAYER + 1, 1);
    set_sprite_tile(SPR_PLAYER + 2, 2);
    set_sprite_tile(SPR_PLAYER + 3, 3);
    set_sprite_prop(SPR_PLAYER, 0);
    set_sprite_prop(SPR_PLAYER + 1, 0);
    set_sprite_prop(SPR_PLAYER + 2, 0);
    set_sprite_prop(SPR_PLAYER + 3, 0);

    /* Zone name at top */
    ui_set_area_palette(0, 0, 20, 1, 0); /* UI palette for text */
    ui_print(0, 0, zone_db[save.current_zone].name);
}

/* =========================================================
   PLAYER MOVEMENT
   Returns 1 if random encounter triggered
   ========================================================= */
uint8_t world_move_player(uint8_t dir) {
    uint8_t new_x = save.player_x;
    uint8_t new_y = save.player_y;
    uint8_t tile;

    save.player_dir = dir;

    switch (dir) {
        case DIR_UP:    if (new_y > 0) new_y--; break;
        case DIR_DOWN:  if (new_y < MAP_H - 1) new_y++; break;
        case DIR_LEFT:  if (new_x > 0) new_x--; break;
        case DIR_RIGHT: if (new_x < MAP_W - 1) new_x++; break;
    }

    tile = map_tiles[new_y * MAP_W + new_x];

    /* Check for sign interaction */
    if (tile == T_SIGN) {
        ui_show_message(zone_db[save.current_zone].name, "Zone");
        ui_wait_button();
        world_draw();
        return 0;
    }

    /* Check for door (shop) */
    if (tile == T_DOOR) {
        world_shop();
        world_draw();
        return 0;
    }

    /* Check for zone exit (right edge path) */
    if (new_x >= MAP_W - 2 && tile == T_PATH) {
        /* Check if boss needs to be beaten first */
        if (save.zones_cleared <= save.current_zone) {
            /* Boss battle! */
            uint8_t won = battle_boss(
                zone_db[save.current_zone].boss_species,
                zone_db[save.current_zone].boss_level
            );
            if (won) {
                save.zones_cleared = save.current_zone + 1;
                save.badges++;
                ui_show_message("Zone cleared!", "Badge earned!");
                ui_wait_button();

                /* Advance to next zone */
                if (save.current_zone < NUM_ZONES - 1) {
                    save.current_zone++;
                    save.player_x = 3;
                    save.player_y = 9;
                    world_generate_zone();
                }
            }
            world_draw();
            return 0;
        } else if (save.current_zone < NUM_ZONES - 1) {
            /* Already cleared, can advance */
            save.current_zone++;
            save.player_x = 3;
            save.player_y = 9;
            world_generate_zone();
            world_draw();
            return 0;
        }
    }

    /* Check walkability */
    if (!is_walkable(tile)) {
        return 0;
    }

    /* Move player */
    save.player_x = new_x;
    save.player_y = new_y;

    /* Update sprite position */
    move_sprite(SPR_PLAYER,     (save.player_x * 8) + 8, (save.player_y * 8) + 16);
    move_sprite(SPR_PLAYER + 1, (save.player_x * 8) + 16, (save.player_y * 8) + 16);
    move_sprite(SPR_PLAYER + 2, (save.player_x * 8) + 8, (save.player_y * 8) + 24);
    move_sprite(SPR_PLAYER + 3, (save.player_x * 8) + 16, (save.player_y * 8) + 24);

    /* Random encounter in tall grass */
    if (tile == T_TALL) {
        if (rng_range(1, 10) <= 3) { /* 30% encounter rate */
            return 1;
        }
    }

    return 0;
}

uint8_t world_get_tile(uint8_t x, uint8_t y) {
    if (x >= MAP_W || y >= MAP_H) return T_ROCK;
    return map_tiles[y * MAP_W + x];
}

void world_update(void) {
    /* Animate water tiles every 32 frames */
    if ((frame_count & 31) == 0) {
        uint16_t i;
        for (i = 0; i < MAP_W * MAP_H; i++) {
            if (map_tiles[i] == T_WATER) map_tiles[i] = T_WATER2;
            else if (map_tiles[i] == T_WATER2) map_tiles[i] = T_WATER;
        }
        /* Redraw only water area if present */
        world_draw();
    }
}

/* =========================================================
   SHOP
   ========================================================= */
void world_shop(void) {
    const char *shop_items[] = {"Potion 30g", "Elixir 80g", "Antidote 20g", "SmokeBall 15g"};
    const uint16_t prices[] = {30, 80, 20, 15};
    const uint8_t item_ids[] = {ITEM_POTION, ITEM_ELIXIR, ITEM_ANTIDOTE, ITEM_SMOKEBALL};
    uint8_t choice;

    ui_clear_screen();
    ui_show_message("Welcome to", "the shop!");
    ui_wait_button();

    while (1) {
        ui_clear_screen();
        ui_print(1, 0, "Shop");
        ui_print(1, 1, "Gold:");
        ui_print_num(7, 1, save.gold, 5);

        ui_draw_box(0, 3, 20, 7);
        choice = ui_menu(1, 4, shop_items, 4);

        if (choice == 0xFF) break;

        if (save.gold >= prices[choice]) {
            save.gold -= prices[choice];
            save.items[item_ids[choice]]++;
            ui_show_message("Bought!", "");
        } else {
            ui_show_message("Not enough", "gold!");
        }
        ui_wait_button();
    }
}
