#ifndef GFX_DATA_H
#define GFX_DATA_H

#include <stdint.h>
#include "common.h"

/* Font tiles (tiles 0-44): blank, A-Z, 0-9, punctuation */
extern const uint8_t font_tiles[];

/* Overworld tiles (tiles 48-63): grass, tall-grass, path, etc. */
extern const uint8_t overworld_tiles[];

/* UI tiles (tiles 64-79): box borders, HP bar, arrows */
extern const uint8_t ui_tiles[];

/* Skill-tree tiles (tiles 80-87): nodes, lines */
extern const uint8_t skilltree_tiles[];

/* Player 16x16 overworld sprite (4 x 8x8 tiles for OAM) */
extern const uint8_t player_sprite[];

/* Creature front sprites – 6 species, each 4x4 tiles (32x32 px).
 * Each entry is 16 tiles * 16 bytes = 256 bytes. */
extern const uint8_t creature_sprites[MAX_SPECIES][256];

/* GBC colour palettes (4 colours each, 2 bytes per colour = 8 bytes per pal) */
extern const uint16_t bg_palettes[8][4];    /* background palettes */
extern const uint16_t spr_palettes[8][4];   /* sprite palettes     */

#endif /* GFX_DATA_H */
