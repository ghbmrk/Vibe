/* creature.h - Creature species, stats, moves */
#ifndef CREATURE_H
#define CREATURE_H

#include "common.h"

/* Species base stat lookup */
extern const SpeciesData species_table[SP_COUNT];
extern const MoveData    move_table[MOVE_COUNT];
extern const char *const species_names[SP_COUNT];
extern const char *const move_names[MOVE_COUNT];
extern const char *const elem_names[ELEM_COUNT];

/* Create a creature of given species at given level */
void creature_create(Creature *c, uint8_t species, uint8_t level);

/* Recalculate stats from base + level + gear + skills */
void creature_calc_stats(Creature *c);

/* Award XP; returns 1 if leveled up */
uint8_t creature_award_xp(Creature *c, uint16_t amount);

/* XP needed for next level */
uint16_t creature_xp_for_level(uint8_t level);

/* Heal creature to full */
void creature_heal(Creature *c);

/* Type effectiveness: returns 150 for super, 67 for not very, 100 for neutral */
uint8_t type_effectiveness(uint8_t atk_type, uint8_t def_type);

/* Get STAB (same-type attack bonus): 125 if match, else 100 */
uint8_t type_stab(uint8_t creature_type, uint8_t move_type);

#endif
