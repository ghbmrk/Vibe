/*  world.c  –  Overworld maps, player movement, and encounter generation.
 *
 *  Each map is a 20x18 tile grid (one screen, no scrolling).
 *  Exits at the edges transition to the next map.
 *  Tall-grass tiles trigger random wild encounters.
 */

#include "world.h"
#include "ui.h"
#include "rng.h"
#include "gfx_data.h"
#include <gb/gb.h>
#include <string.h>

/* ---- Map data (stored in ROM) ----------------------------- */

/*  Tile legend (see common.h for tile indices):
 *   G = TILE_GRASS       g = TILE_TALLGRASS   P = TILE_PATH
 *   W = TILE_WATER       1 = TREE_TL 2 = TREE_TR
 *   3 = TREE_BL 4 = TREE_BR   R = TILE_ROCK
 *   H = TILE_FENCE_H     V = TILE_FENCE_V
 *   D = TILE_DOOR        r = TILE_ROOF   w = TILE_WALL
 *   F = TILE_FLOWER      S = TILE_SIGN
 *
 *   We store maps as uint8_t arrays using the actual tile indices.
 */

#define G  TILE_GRASS
#define g  TILE_TALLGRASS
#define P  TILE_PATH
#define W  TILE_WATER
#define T1 TILE_TREE_TL
#define T2 TILE_TREE_TR
#define T3 TILE_TREE_BL
#define T4 TILE_TREE_BR
#define RK TILE_ROCK
#define FH TILE_FENCE_H
#define FV TILE_FENCE_V
#define DR TILE_DOOR
#define RF TILE_ROOF
#define WL TILE_WALL
#define FL TILE_FLOWER
#define SN TILE_SIGN

/* Map 0 – Starting Village */
static const uint8_t map_village[MAP_H][MAP_W] = {
    { T1,T2,G, G, G, G, G, G, T1,T2,G, G, G, G, G, T1,T2,G, G, G  },
    { T3,T4,G, G, G, G, G, G, T3,T4,G, G, G, G, G, T3,T4,G, G, G  },
    { G, G, G, G, RF,RF,RF,G, G, G, G, G, RF,RF,RF,G, G, G, G, G  },
    { G, G, G, G, WL,DR,WL,G, G, G, G, G, WL,DR,WL,G, G, G, G, G  },
    { G, G, G, G, G, P, G, G, G, G, G, G, G, P, G, G, G, G, G, G  },
    { G, FL,G, G, G, P, G, G, G, FL,G, G, G, P, G, G, G, FL,G, G  },
    { FH,FH,FH,P, P, P, P, P, P, P, P, P, P, P, P, P, FH,FH,FH,FH },
    { G, G, G, P, G, G, G, G, G, G, G, G, G, G, G, P, G, G, G, G  },
    { G, G, G, P, G, G, SN,G, G, G, G, G, G, G, G, P, G, G, G, G  },
    { G, G, G, P, G, G, G, G, G, G, G, G, G, G, G, P, G, G, G, G  },
    { T1,T2,G, P, G, G, G, G, G, g, g, g, G, G, G, P, G, T1,T2,G  },
    { T3,T4,G, P, G, G, G, G, g, g, g, g, g, G, G, P, G, T3,T4,G  },
    { G, G, G, P, G, G, G, G, g, g, g, g, g, G, G, P, G, G, G, G  },
    { G, G, G, P, G, G, G, G, G, g, g, g, G, G, G, P, G, G, G, G  },
    { G, G, G, P, G, G, G, G, G, G, G, G, G, G, G, P, G, G, G, G  },
    { FH,FH,FH,P, P, P, P, P, P, P, P, P, P, P, P, P, FH,FH,FH,FH },
    { G, G, G, G, G, G, G, G, G, P, G, G, G, G, G, G, G, G, G, G  },
    { G, G, G, G, G, G, G, G, G, P, G, G, G, G, G, G, G, G, G, G  },
};

/* Map 1 – Route 1 (grasslands with tall grass) */
static const uint8_t map_route1[MAP_H][MAP_W] = {
    { G, G, G, G, G, G, G, G, G, P, G, G, G, G, G, G, G, G, G, G  },
    { G, G, G, g, g, g, G, G, G, P, G, G, G, g, g, g, G, G, G, G  },
    { G, G, g, g, g, g, g, G, G, P, G, G, g, g, g, g, g, G, G, G  },
    { G, G, g, g, g, g, g, G, G, P, G, G, g, g, g, g, g, G, G, G  },
    { G, G, G, g, g, g, G, G, G, P, G, G, G, g, g, g, G, G, G, G  },
    { G, G, G, G, G, G, G, G, P, P, P, G, G, G, G, G, G, G, G, G  },
    { T1,T2,G, G, G, G, G, G, P, G, P, G, G, G, G, G, G, T1,T2,G  },
    { T3,T4,G, G, G, G, G, G, P, G, P, G, G, G, G, G, G, T3,T4,G  },
    { G, G, G, g, g, g, g, P, P, G, P, P, g, g, g, g, G, G, G, G  },
    { G, G, g, g, g, g, g, P, G, G, G, P, g, g, g, g, g, G, G, G  },
    { G, G, g, g, g, g, g, P, G, RK,G, P, g, g, g, g, g, G, G, G  },
    { G, G, G, g, g, g, G, P, G, G, G, P, G, g, g, g, G, G, G, G  },
    { G, G, G, G, G, G, G, P, P, P, P, P, G, G, G, G, G, G, G, G  },
    { T1,T2,G, G, G, G, G, G, G, P, G, G, G, G, G, G, G, T1,T2,G  },
    { T3,T4,G, g, g, g, G, G, G, P, G, G, G, g, g, g, G, T3,T4,G  },
    { G, G, g, g, g, g, g, G, G, P, G, G, g, g, g, g, g, G, G, G  },
    { G, G, g, g, g, g, g, G, G, P, G, G, g, g, g, g, g, G, G, G  },
    { G, G, G, g, g, g, G, G, G, P, G, G, G, g, g, g, G, G, G, G  },
};

/* Map 2 – Deep Forest (dense encounters, rare creatures) */
static const uint8_t map_forest[MAP_H][MAP_W] = {
    { T1,T2,T1,T2,G, G, G, G, G, P, G, G, G, G, T1,T2,T1,T2,T1,T2 },
    { T3,T4,T3,T4,G, g, g, G, G, P, G, g, g, G, T3,T4,T3,T4,T3,T4 },
    { T1,T2,G, G, g, g, g, g, G, P, G, g, g, g, g, G, G, T1,T2,G  },
    { T3,T4,G, g, g, g, g, g, G, P, G, g, g, g, g, g, G, T3,T4,G  },
    { G, G, g, g, g, g, g, g, P, P, P, g, g, g, g, g, g, G, G, G  },
    { G, G, g, g, g, g, g, P, P, G, P, P, g, g, g, g, g, G, G, G  },
    { T1,T2,g, g, g, g, G, P, G, G, G, P, G, g, g, g, T1,T2,G, G  },
    { T3,T4,G, g, g, G, G, P, G, FL,G, P, G, G, g, g, T3,T4,G, G  },
    { G, G, G, G, G, G, P, P, G, G, G, P, P, G, G, G, G, G, G, G  },
    { T1,T2,G, G, G, P, P, G, G, RK,G, G, P, P, G, G, G, T1,T2,G  },
    { T3,T4,G, G, P, P, G, g, g, G, g, g, G, P, P, G, G, T3,T4,G  },
    { G, G, G, P, P, G, g, g, g, g, g, g, g, G, P, P, G, G, G, G  },
    { G, G, P, P, G, g, g, g, g, g, g, g, g, g, G, P, P, G, G, G  },
    { T1,T2,P, G, g, g, g, g, g, g, g, g, g, g, g, G, P, T1,T2,G  },
    { T3,T4,P, G, G, g, g, g, g, g, g, g, g, g, G, G, P, T3,T4,G  },
    { G, G, P, P, G, G, g, g, g, g, g, g, g, G, G, P, P, G, G, G  },
    { T1,T2,G, P, P, G, G, G, G, P, G, G, G, G, P, P, G, T1,T2,G  },
    { T3,T4,G, G, P, P, P, P, P, P, P, P, P, P, P, G, G, T3,T4,G  },
};

static const uint8_t (*maps[NUM_MAPS])[MAP_W] = {
    map_village, map_route1, map_forest
};

/* Per-map encounter rates (out of 256 per step on tall grass) */
static const uint8_t encounter_rates[NUM_MAPS] = { 15, 30, 45 };

/* Per-map creature level ranges */
static const uint8_t level_min[NUM_MAPS] = { 2, 3, 6 };
static const uint8_t level_max[NUM_MAPS] = { 4, 7, 12 };

/* Per-map available species (indices into species_table) */
static const uint8_t species_pool_0[] = { 0, 1, 2 };
static const uint8_t species_pool_1[] = { 0, 1, 2, 3 };
static const uint8_t species_pool_2[] = { 3, 4, 5 };
static const uint8_t *species_pools[NUM_MAPS] = {
    species_pool_0, species_pool_1, species_pool_2
};
static const uint8_t pool_sizes[NUM_MAPS] = { 3, 4, 3 };

/* Map exit definitions: { direction(0=up,1=down,2=left,3=right), dest_map, dest_x, dest_y } */
#define MAX_EXITS 4
typedef struct { uint8_t dir; uint8_t dest; uint8_t dx; uint8_t dy; } MapExit;

static const MapExit exits_village[] = {
    { DIR_DOWN, 1, 9, 0 },    /* south exit → route 1, top */
};
static const MapExit exits_route1[] = {
    { DIR_UP,   0, 9, 17 },   /* north → village, bottom */
    { DIR_DOWN, 2, 9, 0 },    /* south → forest, top */
};
static const MapExit exits_forest[] = {
    { DIR_UP, 1, 9, 17 },     /* north → route 1, bottom */
};
static const MapExit *exit_table[NUM_MAPS] = {
    exits_village, exits_route1, exits_forest
};
static const uint8_t exit_counts[NUM_MAPS] = { 1, 2, 1 };

/* ---- Collision check -------------------------------------- */

static uint8_t tile_is_solid(uint8_t tile) {
    /* Trees, rocks, water, walls, fences, roofs are solid */
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

/* ---- State ------------------------------------------------ */

static uint8_t move_cooldown;   /* frames until next step allowed */
static uint8_t pal_dirty;      /* 1 = palette attributes need refresh */
#define MOVE_DELAY 6

/* ---- Public functions ------------------------------------- */

/* Assign GBC palette attributes to each tile based on tile type */
static void world_set_palettes(void) {
    uint8_t x, y, tile, pal;
    uint8_t pal_row[MAP_W];

    VBK_REG = 1;
    for (y = 0; y < MAP_H; y++) {
        for (x = 0; x < MAP_W; x++) {
            tile = maps[current_map][y][x];
            switch (tile) {
                case TILE_GRASS:
                case TILE_FLOWER:     pal = 1; break;
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

void world_load(uint8_t map_id) {
    current_map = map_id;
    move_cooldown = 0;
    pal_dirty = 1;
}

uint8_t world_update(void) {
    int8_t dx = 0, dy = 0;
    uint8_t nx, ny, tile;
    uint8_t i;

    if (move_cooldown) { move_cooldown--; return 0; }

    /* Read directional input */
    if (jpad & J_UP)    { dy = -1; player_dir = DIR_UP;    }
    if (jpad & J_DOWN)  { dy =  1; player_dir = DIR_DOWN;  }
    if (jpad & J_LEFT)  { dx = -1; player_dir = DIR_LEFT;  }
    if (jpad & J_RIGHT) { dx =  1; player_dir = DIR_RIGHT; }

    if (dx == 0 && dy == 0) return 0;

    nx = (uint8_t)((int8_t)player_x + dx);
    ny = (uint8_t)((int8_t)player_y + dy);

    /* ---- Map edge / exit check ----------------------------- */
    if (nx >= MAP_W || ny >= MAP_H) {
        /* Check exits */
        for (i = 0; i < exit_counts[current_map]; i++) {
            const MapExit *e = &exit_table[current_map][i];
            if (e->dir == player_dir) {
                world_load(e->dest);
                player_x = e->dx;
                player_y = e->dy;
                return 0;
            }
        }
        return 0;  /* no exit in this direction */
    }

    /* ---- Collision ----------------------------------------- */
    tile = maps[current_map][ny][nx];
    if (tile_is_solid(tile)) return 0;

    /* Move the player */
    player_x = nx;
    player_y = ny;
    move_cooldown = MOVE_DELAY;

    /* ---- Encounter check ----------------------------------- */
    if (tile == TILE_TALLGRASS) {
        if (rng_range(0, 255) < encounter_rates[current_map]) {
            return 1;  /* wild encounter! */
        }
    }

    return 0;
}

void world_render(void) {
    /* Draw map tiles to background */
    set_bkg_tiles(0, 0, MAP_W, MAP_H,
                  (const uint8_t *)maps[current_map]);

    /* Apply palette attributes once after load or state transition */
    if (pal_dirty) {
        world_set_palettes();
        pal_dirty = 0;
    }

    /* Position player sprite (OAM).
     * OAM coordinates are offset by (8, 16) on Game Boy. */
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
