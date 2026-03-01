#ifndef GFX_DATA_H
#define GFX_DATA_H

#include <stdint.h>

/* Background tiles (loaded to VRAM BG) */
extern const uint8_t font_tiles[];       /* 70 tiles: A-Z,a-z,0-9,punc */
extern const uint8_t ui_tiles[];         /* 16 tiles: borders, bars, cursor */
extern const uint8_t terrain_tiles[];    /* 16 tiles: grass, water, tree, etc */

/* Sprite tiles (loaded to VRAM OBJ) */
extern const uint8_t player_sprite[];    /* 4 tiles: 16x16 player */
extern const uint8_t creature_sprites[]; /* 12 creatures x 4 tiles = 48 tiles */

/* CGB palettes */
extern const uint16_t bg_palettes[];     /* 8 palettes x 4 colors */
extern const uint16_t obj_palettes[];    /* 8 palettes x 4 colors */

/* Tile counts */
#define FONT_TILE_COUNT    70
#define UI_TILE_COUNT      16
#define TERRAIN_TILE_COUNT 16
#define PLAYER_TILE_COUNT  4
#define CREATURE_TILE_COUNT 48

void gfx_init(void);

#endif
