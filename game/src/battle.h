#ifndef BATTLE_H
#define BATTLE_H

#include "common.h"

/* Start a wild encounter battle. Returns 1 if player won, 0 if fled/lost */
uint8_t battle_wild(Creature *wild);

/* Start a boss battle. Returns 1 if player won. */
uint8_t battle_boss(uint8_t species, uint8_t level);

#endif
