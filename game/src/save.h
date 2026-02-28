/* save.h - SRAM save/load */
#ifndef SAVE_H
#define SAVE_H

#include "common.h"

/* Save current game state to SRAM */
void save_game(void);

/* Load game state from SRAM. Returns 1 if valid save found. */
uint8_t load_game(void);

/* Clear save data */
void clear_save(void);

#endif
