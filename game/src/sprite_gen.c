/*  sprite_gen.c  –  Procedural creature sprite generation.
 *
 *  Each creature starts with its species' base sprite (the "level 1" look).
 *  Every unlocked skill-tree node adds a visual mutation: spikes, armor,
 *  aura dots, wings, etc.  The mutations are deterministic from the node's
 *  properties, so the same tree state always produces the same sprite.
 *
 *  Mutations are applied as OR operations on the 2bpp tile data, so
 *  features accumulate — higher-level creatures look more complex.
 *
 *  The 4x4 tile grid (32x32 px) is treated as horizontally symmetric:
 *  mutations on left-side tiles are mirrored onto the right side.
 */

#include "sprite_gen.h"
#include "gfx_data.h"
#include <string.h>

/* Working buffer – one creature at a time */
static uint8_t gen_buf[256];

/* ---- Mutation shape data (ROM) ----------------------------- */

/*  8 predefined 8x8 pixel patterns stored as single-plane masks.
 *  Each is 8 bytes (one byte per pixel row).                     */
static const uint8_t mut_shapes[8][8] = {
    /* 0: Horn – small upward spike */
    { 0x18, 0x3C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 1: Side spike – diagonal protrusion */
    { 0xC0, 0xE0, 0x70, 0x38, 0x70, 0xE0, 0xC0, 0x00 },
    /* 2: Armor band – horizontal plate */
    { 0x00, 0x00, 0x7E, 0xFF, 0xFF, 0x7E, 0x00, 0x00 },
    /* 3: Aura dots – scattered highlights */
    { 0x42, 0x00, 0x24, 0x00, 0x42, 0x00, 0x24, 0x00 },
    /* 4: Wing – triangular flare */
    { 0x80, 0xC0, 0xE0, 0xF0, 0xF0, 0xE0, 0xC0, 0x80 },
    /* 5: Fang – downward points */
    { 0x00, 0x00, 0x00, 0x00, 0x66, 0x24, 0x18, 0x00 },
    /* 6: Crest – top decoration */
    { 0x54, 0x7C, 0x38, 0x10, 0x00, 0x00, 0x00, 0x00 },
    /* 7: Tail – curved extension */
    { 0x00, 0x00, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0C },
};

/* Which shapes each skill category uses (indexed by variant = node_idx & 3) */
static const uint8_t cat_shapes[4][4] = {
    { 0, 1, 5, 6 },   /* ATTACK:  horn, spike, fang, crest */
    { 2, 2, 2, 2 },   /* DEFEND:  armor band (different tiles) */
    { 3, 3, 3, 3 },   /* SUPPORT: aura dots (different tiles) */
    { 4, 7, 0, 6 },   /* SPECIAL: wing, tail, horn, crest */
};

/* Which tile in the 4x4 grid each mutation targets.
 *  Grid layout:   [ 0][ 1][ 2][ 3]   head
 *                 [ 4][ 5][ 6][ 7]   upper body
 *                 [ 8][ 9][10][11]   lower body
 *                 [12][13][14][15]   base/feet
 *  Only left-half tiles (cols 0-1) are listed; mirror handles cols 2-3. */
static const uint8_t cat_tiles[4][4] = {
    {  1,  0,  1,  0 },   /* ATTACK:  head area */
    {  5,  9,  5,  9 },   /* DEFEND:  center body */
    {  0, 12,  4,  8 },   /* SUPPORT: edges */
    {  4, 13,  4, 12 },   /* SPECIAL: sides & bottom */
};

/* 2bpp colour used per category:
 *   color 3 (both planes) = dark/solid
 *   color 2 (hi plane)    = medium tone
 *   color 1 (lo plane)    = light/ethereal   */
static const uint8_t cat_color[4] = { 3, 2, 1, 3 };

/* ---- Helpers ------------------------------------------------ */

/* Reverse the bit order of a byte (horizontal mirror) */
static uint8_t mirror_byte(uint8_t b) {
    b = (uint8_t)(((b & 0xF0u) >> 4) | ((b & 0x0Fu) << 4));
    b = (uint8_t)(((b & 0xCCu) >> 2) | ((b & 0x33u) << 2));
    b = (uint8_t)(((b & 0xAAu) >> 1) | ((b & 0x55u) << 1));
    return b;
}

/* OR a shape pattern onto a tile in gen_buf.
 * `color`: bit 0 = set lo plane, bit 1 = set hi plane. */
static void apply_stamp(uint8_t tile_idx, const uint8_t *shape, uint8_t color) {
    uint8_t *tile = &gen_buf[(uint16_t)tile_idx * 16u];
    uint8_t row;
    for (row = 0; row < 8; row++) {
        if (color & 1u) tile[row * 2u]     |= shape[row];
        if (color & 2u) tile[row * 2u + 1u] |= shape[row];
    }
}

/* ---- Public API --------------------------------------------- */

const uint8_t *sprite_gen_build(const Creature *c) {
    uint8_t i, cat, variant;
    uint8_t shape_idx, tile_idx, color;
    uint8_t col, mirror_tile;
    const SkillNode *n;

    /* Start with the species base sprite */
    memcpy(gen_buf, creature_sprites[c->species], 256);

    /* Apply mutations for each unlocked node beyond root */
    for (i = 1; i < c->tree.count; i++) {
        n = &c->tree.nodes[i];
        if (!NODE_UNLOCKED(*n)) continue;

        cat     = n->category;
        variant = i & 3u;

        /* Look up shape, tile, and colour from tables */
        shape_idx = cat_shapes[cat][variant];
        tile_idx  = cat_tiles[cat][variant];
        color     = cat_color[cat];

        /* Apply mutation to the target tile */
        apply_stamp(tile_idx, mut_shapes[shape_idx], color);

        /* Mirror onto the horizontally symmetric tile.
         * Column mapping: 0↔3, 1↔2 */
        col = tile_idx & 3u;
        mirror_tile = (tile_idx & 0xFCu) | (3u - col);
        if (mirror_tile != tile_idx) {
            uint8_t m_shape[8];
            uint8_t r;
            for (r = 0; r < 8; r++) {
                m_shape[r] = mirror_byte(mut_shapes[shape_idx][r]);
            }
            apply_stamp(mirror_tile, m_shape, color);
        }
    }

    return gen_buf;
}
