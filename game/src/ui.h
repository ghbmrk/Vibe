#ifndef UI_H
#define UI_H

#include "common.h"

/* Text rendering */
void ui_print(uint8_t x, uint8_t y, const char *str);
void ui_print_num(uint8_t x, uint8_t y, uint16_t num, uint8_t digits);

/* Window/box drawing */
void ui_draw_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void ui_clear_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void ui_clear_screen(void);

/* Message box at bottom of screen */
void ui_show_message(const char *line1, const char *line2);
void ui_wait_button(void);

/* HP bar rendering */
void ui_draw_hp_bar(uint8_t x, uint8_t y, uint16_t hp, uint16_t max_hp);

/* Menu system - returns selected index, or 0xFF if cancelled */
uint8_t ui_menu(uint8_t x, uint8_t y, const char **items, uint8_t count);
uint8_t ui_yes_no(uint8_t x, uint8_t y);

/* Screen transition effects */
void ui_fade_out(void);
void ui_fade_in(void);

/* Set BG palette for a map area */
void ui_set_area_palette(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pal);

#endif
