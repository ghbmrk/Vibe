/* character.h - Player character (Space Marine) */
#ifndef CHARACTER_H
#define CHARACTER_H

#include "common.h"

/* Initialize character with starting stats */
void character_init(Character *ch);

/* Recalculate derived values from skills + gear */
void character_calc_stats(Character *ch);

/* Award XP; returns 1 if leveled up */
uint8_t character_award_xp(Character *ch, uint16_t amount);

/* XP needed for next level */
uint16_t character_xp_for_level(uint8_t level);

/* Get effective stat with gear + skill bonuses */
uint8_t character_get_cha(const Character *ch);
uint8_t character_get_wis(const Character *ch);
uint8_t character_get_lck(const Character *ch);
uint8_t character_get_con(const Character *ch);

/* Get catch rate bonus from CHA (percentage points added) */
uint8_t character_catch_bonus(const Character *ch);

/* Get gold multiplier from LCK (percentage, 100 = 1x) */
uint16_t character_gold_mult(const Character *ch);

/* Get XP multiplier from CON (percentage, 100 = 1x) */
uint16_t character_xp_mult(const Character *ch);

/* Get gear cost multiplier from WIS (percentage, 100 = 1x) */
uint16_t character_cost_mult(const Character *ch);

/* Check if character has a specific skill */
uint8_t character_has_skill(const Character *ch, uint8_t skill_id);

#endif
