#ifndef UI_H
#define UI_H

#include "common.h"

/* Load font and UI tile data into VRAM.  Call once during init. */
void ui_init(void);

/* Clear the entire background to blank tiles. */
void ui_clear(void);

/* Print a null-terminated string at tile coords (x, y).
 * Supports A-Z, 0-9 and basic punctuation.  Max 20 chars. */
void ui_print(uint8_t x, uint8_t y, const char *str);

/* Print an unsigned 16-bit number at tile coords (x, y).
 * Right-aligned in a field of up to 5 digits. */
void ui_print_num(uint8_t x, uint8_t y, uint16_t num);

/* Draw a bordered text box from (x, y) with size w x h tiles. */
void ui_draw_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/* Draw a horizontal HP bar at (x, y).  10 tiles wide.
 * Fills proportionally to current/max. */
void ui_draw_hp_bar(uint8_t x, uint8_t y, uint16_t current, uint16_t max);

/* Draw the skill-tree visualisation for the given creature.
 * `selected` is the currently highlighted node index. */
void ui_draw_skill_tree(const Creature *c, uint8_t selected);

/* Draw the title screen. */
void ui_draw_title(void);

/* Draw the starter-selection screen.  `sel` = 0..2 */
void ui_draw_starter(uint8_t sel);

/* Draw the in-game menu.  `sel` = highlighted item. */
void ui_draw_game_menu(uint8_t sel);

/* Draw party summary screen.  `sel` = highlighted slot. */
void ui_draw_party(uint8_t sel);

#endif /* UI_H */
