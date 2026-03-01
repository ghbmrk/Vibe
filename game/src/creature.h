#ifndef CREATURE_H
#define CREATURE_H

#include "common.h"

/* Move database */
#define NUM_MOVES 32
extern const MoveData move_db[NUM_MOVES];

/* Species database */
extern const SpeciesData species_db[NUM_SPECIES];

/* Type effectiveness chart: type_chart[atk_type][def_type] */
/* Returns: 0=immune(0x), 5=not very(0.5x), 10=normal(1x), 20=super(2x) */
extern const uint8_t type_chart[NUM_TYPES][NUM_TYPES];

/* Zone database */
extern const ZoneData zone_db[NUM_ZONES];

/* Functions */
void creature_init(Creature *c, uint8_t species, uint8_t level);
void creature_calc_stats(Creature *c);
uint16_t creature_exp_for_level(uint8_t level);
uint8_t creature_check_levelup(Creature *c);
void creature_check_evolution(Creature *c);
void creature_heal_full(Creature *c);
uint8_t creature_get_palette(uint8_t species);

/* Damage calculation (Showdown-style) */
uint16_t calc_damage(BattleSide *attacker, BattleSide *defender, uint8_t move_idx);

/* Type name strings */
const char *type_name(uint8_t type);

#endif
