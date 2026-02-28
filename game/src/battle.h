#ifndef BATTLE_H
#define BATTLE_H

#include "common.h"

/* Start a battle against a wild creature. */
void battle_start(Creature *enemy);

/* Per-frame update – drives the battle state machine.
 * Returns non-zero when the battle is finished. */
uint8_t battle_update(void);

/* Render the current battle frame to the background. */
void battle_render(void);

/* Result of the last completed battle */
#define BATTLE_RESULT_NONE   0
#define BATTLE_RESULT_WIN    1
#define BATTLE_RESULT_LOSE   2
#define BATTLE_RESULT_CATCH  3
#define BATTLE_RESULT_RUN    4
extern uint8_t battle_result;

#endif /* BATTLE_H */
