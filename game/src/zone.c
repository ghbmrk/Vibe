/* zone.c - Procedural zone generation */
#include "zone.h"
#include "rng.h"

static const char *const theme_names[] = {
    "FLAME RIDGE",   "TIDAL SHORE",  "STONE VALLEY",
    "VOLT PLAINS",   "SHADOW MIRE",  "LIGHT SUMMIT"
};

const char *zone_theme_name(uint8_t theme) {
    if (theme < 6) return theme_names[theme];
    return "UNKNOWN";
}

uint8_t zone_encounter_rate(uint8_t zone_num) {
    uint8_t rate = 15 + zone_num * 2;
    if (rate > 40) rate = 40;
    return rate;
}

/* ── Maze carver using random walk ─────────────────────────── */
static void carve_path(uint8_t map[MAP_H][MAP_W],
                       uint8_t sx, uint8_t sy,
                       uint8_t ex, uint8_t ey) {
    uint8_t cx = sx, cy = sy;
    uint16_t steps = 0;

    while ((cx != ex || cy != ey) && steps < 500) {
        map[cy][cx] = TILE_PATH;
        /* Bias towards exit */
        uint8_t dir = rng_range(0, 3);
        if (rng_chance(60)) {
            if (cx < ex) dir = 3;      /* right */
            else if (cx > ex) dir = 2; /* left */
            else if (cy < ey) dir = 0; /* down */
            else dir = 1;              /* up */
        }
        switch (dir) {
            case 0: if (cy < MAP_H - 2) cy++; break;
            case 1: if (cy > 1) cy--; break;
            case 2: if (cx > 1) cx--; break;
            case 3: if (cx < MAP_W - 2) cx++; break;
        }
        steps++;
    }
    map[ey][ex] = TILE_PATH;
}

/* ── Place tall grass patches near paths ───────────────────── */
static void scatter_grass(uint8_t map[MAP_H][MAP_W]) {
    uint8_t x, y;
    for (y = 1; y < MAP_H - 1; y++) {
        for (x = 1; x < MAP_W - 1; x++) {
            if (map[y][x] == TILE_GRASS) {
                /* Check if adjacent to path */
                if (map[y-1][x] == TILE_PATH || map[y+1][x] == TILE_PATH ||
                    map[y][x-1] == TILE_PATH || map[y][x+1] == TILE_PATH) {
                    if (rng_chance(50)) {
                        map[y][x] = TILE_TALL_GRASS;
                    }
                } else if (rng_chance(8)) {
                    map[y][x] = TILE_TALL_GRASS;
                }
            }
        }
    }
}

/* ── Scatter rocks/water for variety ───────────────────────── */
static void scatter_obstacles(uint8_t map[MAP_H][MAP_W]) {
    uint8_t x, y;
    for (y = 2; y < MAP_H - 2; y++) {
        for (x = 2; x < MAP_W - 2; x++) {
            if (map[y][x] == TILE_GRASS && rng_chance(5)) {
                map[y][x] = rng_chance(50) ? TILE_ROCK : TILE_WATER;
            }
        }
    }
}

/* ── Generate overworld zone ───────────────────────────────── */
void zone_generate(ZoneData *z, uint8_t zone_num) {
    uint8_t x, y;

    z->zone_num = zone_num;
    z->theme = zone_num % 6;
    z->base_level = 3 + zone_num * 4;
    if (z->base_level > 45) z->base_level = 45;
    z->boss_defeated = 0;

    /* Seed RNG deterministically per zone */
    rng_seed(1000 + (uint16_t)zone_num * 137);

    /* Fill with walls */
    for (y = 0; y < MAP_H; y++) {
        for (x = 0; x < MAP_W; x++) {
            if (y == 0 || y == MAP_H - 1 || x == 0 || x == MAP_W - 1) {
                z->map[y][x] = TILE_WALL;
            } else {
                z->map[y][x] = TILE_GRASS;
            }
        }
    }

    /* Entry point (left side) */
    z->entry_x = 2;
    z->entry_y = MAP_H / 2;

    /* Gym entrance (right side) */
    z->gym_x = MAP_W - 4;
    z->gym_y = rng_range(4, MAP_H - 5);

    /* Carve main path from entry to gym */
    carve_path(z->map, z->entry_x, z->entry_y, z->gym_x, z->gym_y);

    /* Carve 2-4 additional random paths for exploration */
    {
        uint8_t num_extra = rng_range(2, 4);
        uint8_t i;
        for (i = 0; i < num_extra; i++) {
            uint8_t sx = rng_range(2, MAP_W / 2);
            uint8_t sy = rng_range(2, MAP_H - 3);
            uint8_t ex = rng_range(MAP_W / 2, MAP_W - 3);
            uint8_t ey = rng_range(2, MAP_H - 3);
            carve_path(z->map, sx, sy, ex, ey);
        }
    }

    /* Place tall grass for encounters */
    scatter_grass(z->map);

    /* Add some obstacles for visual variety */
    scatter_obstacles(z->map);

    /* Place gym door */
    z->map[z->gym_y][z->gym_x] = TILE_DOOR;

    /* Place vendor near entry */
    z->map[z->entry_y - 1][z->entry_x + 1] = TILE_VENDOR;

    /* Add some wall clusters for maze feel */
    {
        uint8_t clusters = rng_range(3, 6);
        uint8_t ci;
        for (ci = 0; ci < clusters; ci++) {
            uint8_t cx = rng_range(4, MAP_W - 5);
            uint8_t cy = rng_range(4, MAP_H - 5);
            uint8_t cw = rng_range(2, 4);
            uint8_t ch = rng_range(2, 3);
            uint8_t wx, wy;
            for (wy = cy; wy < cy + ch && wy < MAP_H - 1; wy++) {
                for (wx = cx; wx < cx + cw && wx < MAP_W - 1; wx++) {
                    if (z->map[wy][wx] == TILE_GRASS ||
                        z->map[wy][wx] == TILE_TALL_GRASS) {
                        z->map[wy][wx] = TILE_WALL;
                    }
                }
            }
        }
    }

    /* Ensure entry and gym are still accessible (clear around them) */
    z->map[z->entry_y][z->entry_x] = TILE_PATH;
    z->map[z->entry_y][z->entry_x + 1] = TILE_PATH;
    z->map[z->gym_y][z->gym_x - 1] = TILE_PATH;
}

/* ── Generate gym interior ─────────────────────────────────── */
void zone_generate_gym(ZoneData *z) {
    uint8_t x, y;

    /* Fill with gym floor */
    for (y = 0; y < SCREEN_H; y++) {
        for (x = 0; x < SCREEN_W; x++) {
            if (y == 0 || y == SCREEN_H - 1 ||
                x == 0 || x == SCREEN_W - 1) {
                z->gym_map[y][x] = TILE_WALL;
            } else {
                z->gym_map[y][x] = TILE_GYM_FLOOR;
            }
        }
    }

    /* Entrance at bottom center */
    z->gym_map[SCREEN_H - 1][SCREEN_W / 2] = TILE_DOOR;

    /* Boss platform at top center */
    z->boss_x = SCREEN_W / 2;
    z->boss_y = 2;
    z->gym_map[z->boss_y][z->boss_x] = TILE_BOSS_MARK;

    /* Create a corridor path from entrance to boss */
    for (y = SCREEN_H - 2; y > z->boss_y; y--) {
        z->gym_map[y][SCREEN_W / 2] = TILE_GYM_FLOOR;
    }

    /* Add some tall grass patches in the gym */
    {
        uint8_t patches = rng_range(3, 5);
        uint8_t i;
        for (i = 0; i < patches; i++) {
            uint8_t px = rng_range(2, SCREEN_W - 3);
            uint8_t py = rng_range(3, SCREEN_H - 3);
            uint8_t pw = rng_range(2, 3);
            uint8_t ph = rng_range(1, 2);
            uint8_t gx, gy;
            for (gy = py; gy < py + ph && gy < SCREEN_H - 1; gy++) {
                for (gx = px; gx < px + pw && gx < SCREEN_W - 1; gx++) {
                    if (z->gym_map[gy][gx] == TILE_GYM_FLOOR) {
                        z->gym_map[gy][gx] = TILE_TALL_GRASS;
                    }
                }
            }
        }
    }

    /* Add some wall obstacles */
    {
        uint8_t walls = rng_range(2, 4);
        uint8_t i;
        for (i = 0; i < walls; i++) {
            uint8_t wx = rng_range(2, SCREEN_W - 3);
            uint8_t wy = rng_range(4, SCREEN_H - 4);
            if (wx != SCREEN_W / 2) {  /* don't block main corridor */
                z->gym_map[wy][wx] = TILE_WALL;
                if (wx + 1 < SCREEN_W - 1 && wx + 1 != SCREEN_W / 2)
                    z->gym_map[wy][wx + 1] = TILE_WALL;
            }
        }
    }
}
