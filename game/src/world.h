#ifndef WORLD_H
#define WORLD_H

#include "common.h"

/* Load the current zone map (outside or gym, based on in_gym global). */
void world_load_zone(void);

/* Per-frame update: movement, exit detection, encounter checks.
 * Returns 0 = nothing, 1 = wild encounter, 2 = boss encounter. */
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
