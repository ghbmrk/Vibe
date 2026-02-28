/*  zone.c  –  Procedural zone generation for infinite progression.
 *
 *  Each zone has an outside area and a gym that ends in a boss.
 *  Zones are themed by element type (cycling every 6 zones) and
 *  increase in difficulty.  Maps are deterministic from the zone
 *  number, so regenerating the same zone always yields the same layout.
 */

#include "zone.h"
#include "rng.h"
#include "creature.h"
#include "skilltree.h"
#include <string.h>

/* ---- Local deterministic PRNG (separate from game RNG) ---- */

static uint16_t z_rng;

static uint16_t z_rand(void) {
    z_rng ^= z_rng << 7;
    z_rng ^= z_rng >> 9;
    z_rng ^= z_rng << 8;
    return z_rng;
}

static uint8_t z_range(uint8_t lo, uint8_t hi) {
    if (lo >= hi) return lo;
    return lo + (uint8_t)(z_rand() % (uint16_t)(hi - lo + 1u));
}

/* ---- Generated positions (set by gen functions) ----------- */

static uint8_t g_gym_x, g_gym_y;
static uint8_t g_boss_x, g_boss_y;
static uint8_t g_path_top_x;

/* ---- Theme data ------------------------------------------- */

static const char * const theme_names[NUM_TYPES] = {
    "EMBER", "TIDAL", "STONE", "STORM", "SHADOW", "ASTRAL"
};

/* ---- Public query functions ------------------------------- */

uint8_t zone_get_theme(uint8_t zone) {
    return zone % NUM_TYPES;
}

uint8_t zone_get_base_level(uint8_t zone) {
    uint16_t lv = 3u + (uint16_t)zone * 4u;
    if (lv > MAX_LEVEL - 5u) lv = MAX_LEVEL - 5u;
    return (uint8_t)lv;
}

uint8_t zone_get_boss_level(uint8_t zone) {
    uint16_t lv = (uint16_t)zone_get_base_level(zone) + 5u;
    if (lv > MAX_LEVEL) lv = MAX_LEVEL;
    return (uint8_t)lv;
}

uint8_t zone_get_encounter_rate(uint8_t zone) {
    uint16_t rate = 20u + (uint16_t)zone * 5u;
    if (rate > 80u) rate = 80u;
    return (uint8_t)rate;
}

uint8_t zone_random_species(uint8_t zone) {
    uint8_t theme = zone_get_theme(zone);
    /* 75% chance of theme-matching species, 25% random */
    if (rng_range(0, 3) == 0) {
        return rng_range(0, MAX_SPECIES - 1);
    }
    return theme;
}

const char *zone_theme_name(uint8_t zone) {
    return theme_names[zone_get_theme(zone)];
}

uint8_t zone_gym_door_x(void) { return g_gym_x; }
uint8_t zone_gym_door_y(void) { return g_gym_y; }
uint8_t zone_boss_x(void)     { return g_boss_x; }
uint8_t zone_boss_y(void)     { return g_boss_y; }
uint8_t zone_path_x(void)     { return g_path_top_x; }

/* ---- Map helpers ------------------------------------------ */

static void place(uint8_t *map, uint8_t x, uint8_t y, uint8_t tile) {
    if (x < MAP_W && y < MAP_H) {
        map[(uint16_t)y * MAP_W + x] = tile;
    }
}

static uint8_t get_tile(const uint8_t *map, uint8_t x, uint8_t y) {
    if (x < MAP_W && y < MAP_H) return map[(uint16_t)y * MAP_W + x];
    return TILE_WALL;
}

static void fill_rect(uint8_t *map, uint8_t x, uint8_t y,
                       uint8_t w, uint8_t h, uint8_t tile) {
    uint8_t ix, iy;
    for (iy = 0; iy < h; iy++) {
        for (ix = 0; ix < w; ix++) {
            place(map, x + ix, y + iy, tile);
        }
    }
}

/* ---- Outside map generation ------------------------------- */

void zone_gen_outside(uint8_t *map, uint8_t zone) {
    uint8_t theme = zone_get_theme(zone);
    uint8_t x, y, px, i;
    uint8_t path_xs[MAP_H];
    uint8_t num_patches, feat_count;
    uint8_t bx, by, side;

    /* Seed PRNG deterministically from zone number */
    z_rng = (uint16_t)(zone * 7919u + 12347u);
    if (z_rng == 0) z_rng = 1;

    /* 1. Fill with grass */
    memset(map, TILE_GRASS, (uint16_t)MAP_W * MAP_H);

    /* 2. Tree border at top (leave gap for path at cols 8-11) */
    for (x = 0; x < MAP_W - 1; x += 2) {
        if (x < 8 || x > 11) {
            place(map, x,     0, TILE_TREE_TL);
            place(map, x + 1, 0, TILE_TREE_TR);
            place(map, x,     1, TILE_TREE_BL);
            place(map, x + 1, 1, TILE_TREE_BR);
        }
    }

    /* 3. Main path: random walk from bottom to top */
    px = MAP_W / 2;
    for (y = MAP_H; y > 0; ) {
        y--;
        path_xs[y] = px;
        place(map, px, y, TILE_PATH);
        if (px > 0)           place(map, px - 1, y, TILE_PATH);
        if (px < MAP_W - 1)   place(map, px + 1, y, TILE_PATH);

        /* Horizontal drift (biased toward center) */
        if (y > 1 && y < MAP_H - 1) {
            int8_t drift = (int8_t)z_range(0, 2) - 1;
            if (px < 5)           drift = 1;
            if (px > MAP_W - 6)   drift = -1;
            px = (uint8_t)((int8_t)px + drift);
        }
    }
    g_path_top_x = path_xs[0];

    /* 4. North exit gate: blocked until boss beaten */
    if (!boss_beaten) {
        for (x = 8; x <= 11; x++) {
            place(map, x, 0, TILE_FENCE_H);
            place(map, x, 1, TILE_FENCE_H);
        }
    }

    /* 5. Gym building (roof + wall + door) on one side of path */
    by = z_range(6, 10);
    side = z_rand() & 1;
    if (side) {
        bx = path_xs[by] + z_range(3, 5);
    } else {
        bx = path_xs[by] - z_range(5, 7);
    }
    if (bx > MAP_W - 4) bx = MAP_W - 4;
    if (bx < 1) bx = 1;

    place(map, bx,     by,     TILE_ROOF);
    place(map, bx + 1, by,     TILE_ROOF);
    place(map, bx + 2, by,     TILE_ROOF);
    place(map, bx,     by + 1, TILE_WALL);
    place(map, bx + 1, by + 1, TILE_DOOR);
    place(map, bx + 2, by + 1, TILE_WALL);

    /* Path from main road to gym door */
    {
        uint8_t door_x = bx + 1;
        uint8_t from_x = path_xs[by + 2];
        uint8_t lo, hi;
        place(map, door_x, by + 2, TILE_PATH);
        lo = (door_x < from_x) ? door_x : from_x;
        hi = (door_x > from_x) ? door_x : from_x;
        for (x = lo; x <= hi; x++) {
            place(map, x, by + 2, TILE_PATH);
        }
    }

    /* Sign next to gym */
    if (bx >= 2) place(map, bx - 1, by + 1, TILE_SIGN);

    g_gym_x = bx + 1;
    g_gym_y = by + 2;   /* tile in front of door */

    /* 6. Tall grass patches for encounters */
    num_patches = z_range(4, 6);
    for (i = 0; i < num_patches; i++) {
        uint8_t gx = z_range(1, MAP_W - 5);
        uint8_t gy = z_range(3, MAP_H - 3);
        uint8_t gw = z_range(3, 5);
        uint8_t gh = z_range(2, 4);
        uint8_t tx, ty;
        for (ty = gy; ty < gy + gh && ty < MAP_H; ty++) {
            for (tx = gx; tx < gx + gw && tx < MAP_W; tx++) {
                if (get_tile(map, tx, ty) == TILE_GRASS) {
                    place(map, tx, ty, TILE_TALLGRASS);
                }
            }
        }
    }

    /* 7. Theme-specific features */
    feat_count = z_range(3, 6);
    switch (theme) {
        case TYPE_SHADOW:
            /* Extra trees for dense forest feel */
            for (i = 0; i < feat_count; i++) {
                uint8_t tx = z_range(1, MAP_W - 3);
                uint8_t ty = z_range(3, MAP_H - 3);
                if (get_tile(map, tx, ty) == TILE_GRASS &&
                    get_tile(map, tx + 1, ty) == TILE_GRASS &&
                    get_tile(map, tx, ty + 1) == TILE_GRASS &&
                    get_tile(map, tx + 1, ty + 1) == TILE_GRASS) {
                    place(map, tx,     ty,     TILE_TREE_TL);
                    place(map, tx + 1, ty,     TILE_TREE_TR);
                    place(map, tx,     ty + 1, TILE_TREE_BL);
                    place(map, tx + 1, ty + 1, TILE_TREE_BR);
                }
            }
            break;
        default: {
            uint8_t feat_tile;
            switch (theme) {
                case TYPE_FLAME:  feat_tile = TILE_ROCK;   break;
                case TYPE_AQUA:   feat_tile = TILE_WATER;  break;
                case TYPE_TERRA:  feat_tile = TILE_ROCK;   break;
                default:          feat_tile = TILE_FLOWER; break;
            }
            for (i = 0; i < feat_count; i++) {
                uint8_t fx = z_range(1, MAP_W - 2);
                uint8_t fy = z_range(3, MAP_H - 2);
                if (get_tile(map, fx, fy) == TILE_GRASS) {
                    place(map, fx, fy, feat_tile);
                    if (feat_tile == TILE_WATER || feat_tile == TILE_ROCK) {
                        if (get_tile(map, fx + 1, fy) == TILE_GRASS)
                            place(map, fx + 1, fy, feat_tile);
                        if (get_tile(map, fx, fy + 1) == TILE_GRASS)
                            place(map, fx, fy + 1, feat_tile);
                    }
                }
            }
            break;
        }
    }

    /* 8. Side tree borders (scattered) */
    for (y = 2; y < MAP_H; y += z_range(2, 4)) {
        if (z_range(0, 2) == 0 && y + 1 < MAP_H) {
            place(map, 0, y, TILE_TREE_TL);
            place(map, 1, y, TILE_TREE_TR);
            place(map, 0, y + 1, TILE_TREE_BL);
            place(map, 1, y + 1, TILE_TREE_BR);
        }
        if (z_range(0, 2) == 0 && y + 1 < MAP_H) {
            place(map, MAP_W - 2, y, TILE_TREE_TL);
            place(map, MAP_W - 1, y, TILE_TREE_TR);
            place(map, MAP_W - 2, y + 1, TILE_TREE_BL);
            place(map, MAP_W - 1, y + 1, TILE_TREE_BR);
        }
    }
}

/* ---- Gym map generation ----------------------------------- */

void zone_gen_gym(uint8_t *map, uint8_t zone) {
    uint8_t theme = zone_get_theme(zone);
    uint8_t cx, cy, i;
    uint8_t num_rooms;

    z_rng = (uint16_t)(zone * 3571u + 8423u);
    if (z_rng == 0) z_rng = 1;

    /* 1. Fill with wall */
    memset(map, TILE_WALL, (uint16_t)MAP_W * MAP_H);

    /* 2. Entrance at south center */
    place(map, 9, MAP_H - 1, TILE_DOOR);
    fill_rect(map, 8, MAP_H - 3, 4, 2, TILE_PATH);

    /* 3. Main corridor from entrance to boss room */
    cx = 9;
    for (cy = MAP_H - 4; cy > 4; cy--) {
        fill_rect(map, cx - 1, cy, 3, 1, TILE_PATH);
        /* Occasional bend */
        if (z_range(0, 3) == 0 && cy > 6) {
            int8_t shift = (z_rand() & 1) ? 3 : -3;
            uint8_t new_cx = (uint8_t)((int8_t)cx + shift);
            if (new_cx >= 4 && new_cx <= MAP_W - 5) {
                /* Horizontal connector */
                uint8_t lo = (cx < new_cx) ? (cx - 1) : (new_cx - 1);
                uint8_t hi = (cx > new_cx) ? (cx + 1) : (new_cx + 1);
                fill_rect(map, lo, cy, hi - lo + 1, 1, TILE_PATH);
                cx = new_cx;
            }
        }
    }

    /* 4. Side rooms */
    num_rooms = z_range(2, 3);
    for (i = 0; i < num_rooms; i++) {
        uint8_t ry = z_range(6, MAP_H - 6);
        uint8_t side = z_rand() & 1;
        uint8_t rx, rw, rh;
        rw = z_range(4, 6);
        rh = z_range(3, 4);

        if (side) {
            rx = cx + 2;
        } else {
            rx = (cx > rw + 1) ? (cx - rw - 1) : 1;
        }
        if (rx + rw >= MAP_W) rw = MAP_W - 1 - rx;
        if (rx < 1) rx = 1;
        if (ry + rh >= MAP_H - 1) rh = MAP_H - 2 - ry;

        fill_rect(map, rx, ry, rw, rh, TILE_PATH);

        /* Connect room to corridor */
        {
            uint8_t conn_y = ry + rh / 2;
            uint8_t lo = MIN(rx, cx - 1);
            uint8_t hi = MAX(rx + rw - 1, cx + 1);
            fill_rect(map, lo, conn_y, hi - lo + 1, 1, TILE_PATH);
        }

        /* Theme decoration */
        switch (theme) {
            case TYPE_AQUA:
                if (rw > 3 && rh > 2) {
                    place(map, rx + 1, ry + 1, TILE_WATER);
                    place(map, rx + 2, ry + 1, TILE_WATER);
                }
                break;
            case TYPE_FLAME:
            case TYPE_TERRA:
                place(map, rx + rw / 2, ry + rh / 2, TILE_ROCK);
                break;
            default:
                if (rw > 2 && rh > 2)
                    place(map, rx + 1, ry + 1, TILE_FLOWER);
                break;
        }
    }

    /* 5. Boss room at top */
    fill_rect(map, 5, 1, 10, 4, TILE_PATH);
    /* Connect boss room to corridor top */
    fill_rect(map, MIN(cx - 1, 5), 4, MAX(cx + 2, 15) - MIN(cx - 1, 5), 1, TILE_PATH);

    /* Boss marker */
    g_boss_x = 9;
    g_boss_y = 2;
    place(map, g_boss_x, g_boss_y, TILE_SIGN);
    place(map, g_boss_x - 1, g_boss_y, TILE_FLOWER);
    place(map, g_boss_x + 1, g_boss_y, TILE_FLOWER);
}

/* ---- Boss creation ---------------------------------------- */

void zone_create_boss(Creature *boss, uint8_t zone) {
    uint8_t species = zone_get_theme(zone);
    uint8_t level = zone_get_boss_level(zone);
    uint16_t seed = (uint16_t)(zone * 9973u + 777u);

    creature_init(boss, species, level, seed);
    /* Boss gets maximum skill tree development */
    skilltree_auto_unlock(&boss->tree, level, seed);
    creature_calc_stats(boss);
    boss->hp = boss->max_hp;
    boss->skill_pts = 0;
}
