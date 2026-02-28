/* world.c - Overworld rendering and movement */
#include "world.h"
#include "ui.h"
#include "gfx_data.h"

/* ── Camera calculation ────────────────────────────────────── */
static uint8_t cam_x, cam_y;

static void update_camera(void) {
    int16_t cx, cy;

    if (game.in_gym) {
        /* Gym is single-screen, no scrolling */
        cam_x = 0;
        cam_y = 0;
        return;
    }

    cx = (int16_t)game.px * 8 - 80;
    cy = (int16_t)game.py * 8 - 72;

    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    if (cx > (MAP_W - SCREEN_W) * 8) cx = (MAP_W - SCREEN_W) * 8;
    if (cy > (MAP_H - SCREEN_H) * 8) cy = (MAP_H - SCREEN_H) * 8;

    cam_x = (uint8_t)cx;
    cam_y = (uint8_t)cy;
}

/* ── Get palette for a terrain tile ────────────────────────── */
static uint8_t tile_palette(uint8_t tile) {
    switch (tile) {
        case TILE_GRASS:
        case TILE_TALL_GRASS: return PAL_GRASS;
        case TILE_WALL:
        case TILE_ROCK:       return PAL_TREE;
        case TILE_PATH:       return PAL_PATH;
        case TILE_WATER:      return PAL_WATER;
        case TILE_GYM_FLOOR:
        case TILE_BOSS_MARK:  return PAL_GYM;
        case TILE_DOOR:
        case TILE_VENDOR:     return PAL_ACCENT;
        default:              return PAL_UI;
    }
}

/* ── Render overworld map ──────────────────────────────────── */
void world_render_map(void) {
    uint8_t x, y;
    uint8_t row_tiles[MAP_W];
    uint8_t row_attrs[MAP_W];

    for (y = 0; y < MAP_H; y++) {
        for (x = 0; x < MAP_W; x++) {
            row_tiles[x] = game.zone.map[y][x];
            row_attrs[x] = tile_palette(game.zone.map[y][x]);
        }
        set_bkg_tiles(0, y, MAP_W, 1, row_tiles);
        VBK_REG = 1;
        set_bkg_tiles(0, y, MAP_W, 1, row_attrs);
        VBK_REG = 0;
    }

    update_camera();
    SCX_REG = cam_x;
    SCY_REG = cam_y;
}

/* ── Render gym map ────────────────────────────────────────── */
void world_render_gym(void) {
    uint8_t x, y;
    uint8_t row_tiles[SCREEN_W];
    uint8_t row_attrs[SCREEN_W];

    for (y = 0; y < SCREEN_H; y++) {
        for (x = 0; x < SCREEN_W; x++) {
            row_tiles[x] = game.zone.gym_map[y][x];
            row_attrs[x] = tile_palette(game.zone.gym_map[y][x]);
        }
        set_bkg_tiles(0, y, SCREEN_W, 1, row_tiles);
        VBK_REG = 1;
        set_bkg_tiles(0, y, SCREEN_W, 1, row_attrs);
        VBK_REG = 0;
    }

    SCX_REG = 0;
    SCY_REG = 0;
}

/* ── Show player sprite ────────────────────────────────────── */
void world_show_player(void) {
    uint8_t base_tile;

    switch (game.dir) {
        case DIR_UP:    base_tile = SPR_TILE_PLAYER_UP; break;
        case DIR_LEFT:
        case DIR_RIGHT: base_tile = SPR_TILE_PLAYER_SIDE; break;
        default:        base_tile = SPR_TILE_PLAYER_DOWN; break;
    }

    /* 2x2 meta-sprite using OAM sprites 0-3 */
    set_sprite_tile(0, base_tile);
    set_sprite_tile(1, base_tile + 1);
    set_sprite_tile(2, base_tile + 2);
    set_sprite_tile(3, base_tile + 3);

    /* Set sprite properties (flip for left-facing) */
    if (game.dir == DIR_LEFT) {
        set_sprite_prop(0, S_FLIPX);
        set_sprite_prop(1, S_FLIPX);
        set_sprite_prop(2, S_FLIPX);
        set_sprite_prop(3, S_FLIPX);
        /* Swap left/right tiles for proper mirroring */
        set_sprite_tile(0, base_tile + 1);
        set_sprite_tile(1, base_tile);
        set_sprite_tile(2, base_tile + 3);
        set_sprite_tile(3, base_tile + 2);
    } else {
        set_sprite_prop(0, 0);
        set_sprite_prop(1, 0);
        set_sprite_prop(2, 0);
        set_sprite_prop(3, 0);
    }
}

/* ── Hide player sprite ────────────────────────────────────── */
void world_hide_player(void) {
    move_sprite(0, 0, 0);
    move_sprite(1, 0, 0);
    move_sprite(2, 0, 0);
    move_sprite(3, 0, 0);
}

/* ── Update player movement, returns tile stepped onto ─────── */
uint8_t world_update(void) {
    uint8_t pressed = ui_poll_keys();
    uint8_t nx = game.px, ny = game.py;
    uint8_t max_x, max_y;
    uint8_t dest_tile;
    uint8_t screen_x, screen_y;

    if (game.in_gym) {
        max_x = SCREEN_W - 1;
        max_y = SCREEN_H - 1;
    } else {
        max_x = MAP_W - 1;
        max_y = MAP_H - 1;
    }

    if (pressed & J_UP) {
        game.dir = DIR_UP;
        if (ny > 1) ny--;
    } else if (pressed & J_DOWN) {
        game.dir = DIR_DOWN;
        if (ny < max_y - 1) ny++;
    } else if (pressed & J_LEFT) {
        game.dir = DIR_LEFT;
        if (nx > 1) nx--;
    } else if (pressed & J_RIGHT) {
        game.dir = DIR_RIGHT;
        if (nx < max_x - 1) nx++;
    } else if (pressed & J_START) {
        return 0xFE; /* menu request */
    }

    if (nx == game.px && ny == game.py) return 0xFF; /* no movement */

    /* Collision check */
    if (game.in_gym) {
        dest_tile = game.zone.gym_map[ny][nx];
    } else {
        dest_tile = game.zone.map[ny][nx];
    }

    if (dest_tile == TILE_WALL || dest_tile == TILE_ROCK ||
        dest_tile == TILE_WATER) {
        return 0xFF; /* blocked */
    }

    /* Move player */
    game.px = nx;
    game.py = ny;

    /* Update camera */
    update_camera();
    SCX_REG = cam_x;
    SCY_REG = cam_y;

    /* Update sprite position */
    world_show_player();
    screen_x = game.px * 8 - cam_x + 8;
    screen_y = game.py * 8 - cam_y + 16;

    /* 2x2 meta-sprite positioning */
    move_sprite(0, screen_x,     screen_y);
    move_sprite(1, screen_x + 8, screen_y);
    move_sprite(2, screen_x,     screen_y + 8);
    move_sprite(3, screen_x + 8, screen_y + 8);

    return dest_tile;
}

void world_set_tile_palettes(void) {
    /* Palettes are set during render_map/render_gym */
}
