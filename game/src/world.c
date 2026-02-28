/*  world.c  –  Overworld rendering, movement, and zone transitions.
 *
 *  Maps are procedurally generated per-zone by zone.c.
 *  Each zone has an outside area and a gym (inside area).
 *  The gym ends with a boss fight that unlocks the next zone.
 */

#include "world.h"
#include "zone.h"
#include "ui.h"
#include "rng.h"
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>

/* Generated map data (RAM buffer, overwritten each load) */
static uint8_t gen_map[MAP_H][MAP_W];

static uint8_t move_cooldown;
static uint8_t pal_dirty;
static uint8_t at_boss;   /* prevents boss re-trigger when standing on tile */
#define MOVE_DELAY 6

/* ---- Collision check -------------------------------------- */

static uint8_t tile_is_solid(uint8_t tile) {
    if (tile == TILE_TREE_TL || tile == TILE_TREE_TR ||
        tile == TILE_TREE_BL || tile == TILE_TREE_BR) return 1;
    if (tile == TILE_ROCK)    return 1;
    if (tile == TILE_WATER)   return 1;
    if (tile == TILE_WALL)    return 1;
    if (tile == TILE_ROOF)    return 1;
    if (tile == TILE_FENCE_H) return 1;
    if (tile == TILE_FENCE_V) return 1;
    return 0;
}

/* ---- Zone-themed palette attributes ----------------------- */

static void world_set_palettes(void) {
    uint8_t x, y, tile, pal;
    uint8_t pal_row[MAP_W];
    uint8_t theme = zone_get_theme(current_zone);

    /* Theme accent palette for grass tiles */
    uint8_t grass_pal;
    switch (theme) {
        case TYPE_FLAME:  grass_pal = 5; break;
        case TYPE_AQUA:   grass_pal = 4; break;
        case TYPE_TERRA:  grass_pal = 3; break;
        case TYPE_VOLT:   grass_pal = 7; break;
        case TYPE_SHADOW: grass_pal = 6; break;
        default:          grass_pal = 1; break;
    }

    VBK_REG = 1;
    for (y = 0; y < MAP_H; y++) {
        for (x = 0; x < MAP_W; x++) {
            tile = gen_map[y][x];
            switch (tile) {
                case TILE_GRASS:
                case TILE_FLOWER:     pal = grass_pal; break;
                case TILE_TALLGRASS:  pal = 2; break;
                case TILE_PATH:
                case TILE_DOOR:
                case TILE_SIGN:
                case TILE_ROOF:
                case TILE_WALL:       pal = 3; break;
                case TILE_WATER:      pal = 4; break;
                case TILE_TREE_TL:
                case TILE_TREE_TR:
                case TILE_TREE_BL:
                case TILE_TREE_BR:    pal = 2; break;
                default:              pal = 0; break;
            }
            pal_row[x] = pal;
        }
        set_bkg_tiles(0, y, MAP_W, 1, pal_row);
    }
    VBK_REG = 0;
}

/* ---- Public functions ------------------------------------- */

void world_load_zone(void) {
    if (in_gym) {
        zone_gen_gym((uint8_t *)gen_map, current_zone);
    } else {
        zone_gen_outside((uint8_t *)gen_map, current_zone);
    }
    move_cooldown = 0;
    pal_dirty = 1;
    at_boss = 0;
}

uint8_t world_update(void) {
    int8_t dx = 0, dy = 0;
    uint8_t nx, ny, tile;

    if (move_cooldown) { move_cooldown--; return 0; }

    /* Read directional input */
    if (jpad & J_UP)    { dy = -1; player_dir = DIR_UP;    }
    if (jpad & J_DOWN)  { dy =  1; player_dir = DIR_DOWN;  }
    if (jpad & J_LEFT)  { dx = -1; player_dir = DIR_LEFT;  }
    if (jpad & J_RIGHT) { dx =  1; player_dir = DIR_RIGHT; }

    if (dx == 0 && dy == 0) return 0;

    nx = (uint8_t)((int8_t)player_x + dx);
    ny = (uint8_t)((int8_t)player_y + dy);

    /* ---- Map edge check ------------------------------------ */
    if (nx >= MAP_W || ny >= MAP_H) {
        /* Going north off outside map with boss beaten → advance zone */
        if (dy < 0 && !in_gym && boss_beaten) {
            current_zone++;
            boss_beaten = 0;
            world_load_zone();
            player_x = MAP_W / 2;
            player_y = MAP_H - 2;
            return 0;
        }
        return 0;  /* blocked at all other edges */
    }

    /* ---- Collision ----------------------------------------- */
    tile = gen_map[ny][nx];
    if (tile_is_solid(tile)) return 0;

    /* Move the player */
    player_x = nx;
    player_y = ny;
    move_cooldown = MOVE_DELAY;

    /* ---- Door transitions ---------------------------------- */
    if (tile == TILE_DOOR) {
        if (in_gym) {
            /* Exit gym → return to outside at gym entrance */
            in_gym = 0;
            world_load_zone();
            player_x = zone_gym_door_x();
            player_y = zone_gym_door_y();
            return 0;
        } else {
            /* Enter gym from outside */
            in_gym = 1;
            world_load_zone();
            player_x = 9;
            player_y = MAP_H - 4;
            return 0;
        }
    }

    /* ---- Boss encounter (gym only) ------------------------- */
    if (in_gym && !boss_beaten &&
        player_x == zone_boss_x() && player_y == zone_boss_y()) {
        if (!at_boss) {
            at_boss = 1;
            return 2;  /* boss encounter */
        }
    } else {
        at_boss = 0;
    }

    /* ---- Wild encounter check ------------------------------ */
    if (tile == TILE_TALLGRASS) {
        if (rng_range(0, 255) < zone_get_encounter_rate(current_zone)) {
            return 1;  /* wild encounter */
        }
    }

    return 0;
}

void world_render(void) {
    /* Draw generated map to background */
    set_bkg_tiles(0, 0, MAP_W, MAP_H, (const uint8_t *)gen_map);

    /* Apply palette attributes on first render after load */
    if (pal_dirty) {
        world_set_palettes();
        pal_dirty = 0;
    }

    /* Position player sprite (OAM coordinates offset by 8,16) */
    move_sprite(SPR_PLAYER_0, player_x * 8u + 8u,      player_y * 8u + 16u);
    move_sprite(SPR_PLAYER_1, player_x * 8u + 8u + 8u,  player_y * 8u + 16u);
    move_sprite(SPR_PLAYER_2, player_x * 8u + 8u,      player_y * 8u + 16u + 8u);
    move_sprite(SPR_PLAYER_3, player_x * 8u + 8u + 8u,  player_y * 8u + 16u + 8u);
}

void world_hide_player(void) {
    move_sprite(SPR_PLAYER_0, 0, 0);
    move_sprite(SPR_PLAYER_1, 0, 0);
    move_sprite(SPR_PLAYER_2, 0, 0);
    move_sprite(SPR_PLAYER_3, 0, 0);
}

void world_show_player(void) {
    /* world_render will reposition them */
}

void world_mark_dirty(void) {
    pal_dirty = 1;
}
