/* battle.h - Turn-based battle system */
#ifndef BATTLE_H
#define BATTLE_H

#include "common.h"

/* Battle result codes */
#define BATTLE_WIN      0
#define BATTLE_LOSE     1
#define BATTLE_RUN      2

/* Post-battle choice */
#define CHOICE_EAT      0
#define CHOICE_FEED     1
#define CHOICE_CATCH    2

/* Initialize battle state with an enemy creature */
void battle_init(Creature *enemy);

/* Run the full battle loop. Returns BATTLE_WIN/LOSE/RUN. */
uint8_t battle_run(void);

/* Draw the battle screen layout */
void battle_draw_scene(const Creature *player_c, const Creature *enemy_c);

/* Show post-battle choice menu. Returns CHOICE_EAT/FEED/CATCH. */
uint8_t battle_post_choice(const Creature *enemy);

#endif
