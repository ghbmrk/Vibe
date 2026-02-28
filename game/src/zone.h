#ifndef ZONE_H
#define ZONE_H

#include "common.h"

/* Get the element theme for a zone (cycles every NUM_TYPES) */
uint8_t zone_get_theme(uint8_t zone);

/* Base creature level for encounters in this zone */
uint8_t zone_get_base_level(uint8_t zone);

/* Boss creature level */
uint8_t zone_get_boss_level(uint8_t zone);

/* Wild encounter rate (out of 256 per step on tall grass) */
uint8_t zone_get_encounter_rate(uint8_t zone);

/* Pick a random wild species for the zone */
uint8_t zone_random_species(uint8_t zone);

/* Generate the outside map for a zone (MAP_H*MAP_W bytes). */
void zone_gen_outside(uint8_t *map, uint8_t zone);

/* Generate the gym map for a zone (MAP_H*MAP_W bytes). */
void zone_gen_gym(uint8_t *map, uint8_t zone);

/* Positions set by the most recent gen call */
uint8_t zone_gym_door_x(void);
uint8_t zone_gym_door_y(void);
uint8_t zone_boss_x(void);
uint8_t zone_boss_y(void);
uint8_t zone_path_x(void);   /* main path x at top of outside map */

/* Create a boss creature for a zone */
void zone_create_boss(Creature *boss, uint8_t zone);

/* Theme display name (max 6 chars) */
const char *zone_theme_name(uint8_t zone);

#endif /* ZONE_H */
