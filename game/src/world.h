#ifndef WORLD_H
#define WORLD_H

#include "common.h"

/* Generate and draw the current zone map */
void world_generate_zone(void);
void world_draw(void);
void world_update(void);

/* Handle player movement, returns 1 if encounter triggered */
uint8_t world_move_player(uint8_t dir);

/* Check tile at position */
uint8_t world_get_tile(uint8_t x, uint8_t y);

/* Shop interaction */
void world_shop(void);

#endif
