#ifndef SAVE_H
#define SAVE_H

#include "common.h"

/* Check whether a valid save exists in SRAM. */
uint8_t save_exists(void);

/* Write current game state to SRAM. */
void save_game(void);

/* Load game state from SRAM.  Returns 1 on success, 0 on failure. */
uint8_t load_game(void);

#endif /* SAVE_H */
