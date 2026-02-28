/* gfx_data.h - Graphics data declarations */
#ifndef GFX_DATA_H
#define GFX_DATA_H

#include <stdint.h>

/* ── Font ──────────────────────────────────────────────────── */
#define FONT_CHARS    59    /* space, A-Z, 0-9, punctuation */
extern const uint8_t font_1bpp[];   /* 8 bytes per char, 1bpp */

/* ── Terrain tiles (2bpp, 16 bytes each) ───────────────────── */
#define NUM_TERRAIN_TILES  10
extern const uint8_t terrain_tiles[];

/* ── UI tiles (2bpp, 16 bytes each) ────────────────────────── */
#define NUM_UI_TILES  13
extern const uint8_t ui_tiles[];

/* ── Player sprite (2bpp, 16 bytes per 8x8 tile) ──────────── */
/* 3 directions x 4 tiles (2x2) = 12 tiles */
extern const uint8_t player_sprite[];

/* ── Creature battle sprites (2bpp, 4x4 tiles each) ───────── */
/* 6 species x 16 tiles x 16 bytes = 1536 bytes per species */
extern const uint8_t creature_sprites[];  /* [species][tile][byte] */

/* ── Palettes (4 colors each, uint16_t RGB555) ─────────────── */
extern const uint16_t bg_palettes[];   /* 8 palettes x 4 colors */
extern const uint16_t sprite_palettes[];

/* ── Element-themed palettes for battle ────────────────────── */
extern const uint16_t elem_palettes[];  /* 7 elem x 4 colors */

/* ── Functions ─────────────────────────────────────────────── */
void gfx_load_font(void);
void gfx_load_terrain(void);
void gfx_load_ui_tiles(void);
void gfx_load_player_sprite(void);
void gfx_load_creature_sprite(uint8_t dest_tile, uint8_t species);
void gfx_set_overworld_palettes(void);
void gfx_set_battle_palettes(uint8_t player_elem, uint8_t enemy_elem);

#endif
