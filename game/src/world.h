/* world.h - Overworld rendering and player movement */
#ifndef WORLD_H
#define WORLD_H

#include "common.h"

/* Render the visible portion of the current map */
void world_render_map(void);

/* Render the gym interior (single screen) */
void world_render_gym(void);

/* Update player position and camera. Returns tile stepped onto. */
uint8_t world_update(void);

/* Set up player sprite on screen */
void world_show_player(void);

/* Hide player sprite (for battles/menus) */
void world_hide_player(void);

/* Set terrain palette attributes for visible tiles */
void world_set_tile_palettes(void);

#endif
