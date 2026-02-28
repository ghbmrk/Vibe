#ifndef WORLD_H
#define WORLD_H

#include "common.h"

/* Number of maps */
#define NUM_MAPS 3

/* Load a map screen and place the player. */
void world_load(uint8_t map_id);

/* Per-frame update: movement, exit detection, encounter checks.
 * Returns 1 if a wild encounter should trigger. */
uint8_t world_update(void);

/* Render the overworld (background tiles + player sprite). */
void world_render(void);

/* Hide the player sprite (call when leaving overworld). */
void world_hide_player(void);

/* Show the player sprite (call when re-entering overworld). */
void world_show_player(void);

/* Mark palette attributes as needing refresh (call after leaving menus/battle). */
void world_mark_dirty(void);

#endif /* WORLD_H */
