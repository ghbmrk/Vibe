/* skilltree.h - Procedural skill tree (pick 1 of 3) */
#ifndef SKILLTREE_H
#define SKILLTREE_H

#include "common.h"

/* Skill option displayed during level-up */
typedef struct {
    uint8_t id;          /* skill ID (CSKILL_ or MSKILL_) */
    uint8_t is_move;     /* 1 if this teaches a new move (creature only) */
    uint8_t move_id;     /* move to learn if is_move */
} SkillOption;

/* Generate 3 skill options for character level-up */
void skilltree_gen_char_options(SkillOption opts[3]);

/* Generate 3 skill options for creature level-up */
void skilltree_gen_creature_options(SkillOption opts[3], const Creature *c);

/* Apply chosen character skill */
void skilltree_apply_char_skill(Character *ch, const SkillOption *opt);

/* Apply chosen creature skill */
void skilltree_apply_creature_skill(Creature *c, const SkillOption *opt);

/* Get description string for a character skill */
const char *skilltree_char_desc(uint8_t skill_id);

/* Get description string for a creature skill/move option */
const char *skilltree_creature_desc(const SkillOption *opt);

#endif
