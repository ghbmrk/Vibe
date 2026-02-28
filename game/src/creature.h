#ifndef CREATURE_H
#define CREATURE_H

#include "common.h"

/* Species base data table (ROM) */
extern const SpeciesData species_table[MAX_SPECIES];

/* Human-readable type names */
extern const char *type_names[NUM_TYPES];

/* Create a creature of given species / level with a random tree seed. */
void creature_init(Creature *c, uint8_t species, uint8_t level, uint16_t seed);

/* Recalculate derived stats (HP, ATK, DEF, SPD, SPC) from level + base. */
void creature_calc_stats(Creature *c);

/* Award experience; may trigger one or more level-ups. */
void creature_gain_exp(Creature *c, uint16_t amount);

/* Perform a single level-up.  Returns new level. */
uint8_t creature_level_up(Creature *c);

/* Experience required to reach a given level. */
uint16_t creature_exp_for_level(uint8_t level);

/* Type matchup: returns 0 = resisted, 1 = normal, 2 = super effective. */
uint8_t type_effectiveness(uint8_t atk_type, uint8_t def_type);

#endif /* CREATURE_H */
