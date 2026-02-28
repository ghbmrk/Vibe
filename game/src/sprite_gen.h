#ifndef SPRITE_GEN_H
#define SPRITE_GEN_H

#include "common.h"

/* Generate a procedural sprite for a creature based on its species
 * base shape + skill tree unlock state.
 * Returns pointer to an internal 256-byte buffer (16 tiles, 2bpp).
 * The buffer is overwritten on each call. */
const uint8_t *sprite_gen_build(const Creature *c);

#endif /* SPRITE_GEN_H */
