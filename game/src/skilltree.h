#ifndef SKILLTREE_H
#define SKILLTREE_H

#include "common.h"

/* Procedurally generate a skill tree for the given element type + seed. */
void skilltree_generate(SkillTree *tree, uint8_t creature_type, uint16_t seed);

/* Can a node be unlocked?  Checks parent-unlocked, level requirement, etc. */
uint8_t skilltree_can_unlock(const SkillTree *tree, uint8_t node_idx,
                             uint8_t creature_level);

/* Mark a node as unlocked. */
void skilltree_unlock(SkillTree *tree, uint8_t node_idx);

/* Gather indices of unlocked combat-usable skills into `out`.
 * Returns number written (capped at `max`). */
uint8_t skilltree_get_usable(const SkillTree *tree, uint8_t *out, uint8_t max);

/* Get a display name for a skill (writes into buf, max SKILL_NAME_LEN). */
void skilltree_skill_name(char *buf, uint8_t element, uint8_t category,
                          uint8_t variant);

/* Category short tag for display: "ATK" "DEF" "SUP" "SPC" */
const char *skilltree_cat_tag(uint8_t category);

/* Human-readable keystone type name (short, for UI). */
const char *skilltree_ntype_name(uint8_t ntype);

/* Short effect description for a keystone type. */
const char *skilltree_ntype_desc(uint8_t ntype);

/* Auto-unlock nodes for a wild creature.
 * Simulates skill point spending up to `level` using `seed` for choices.
 * Call after skilltree_generate and before recalculating stats. */
void skilltree_auto_unlock(SkillTree *tree, uint8_t level, uint16_t seed);

#endif /* SKILLTREE_H */
