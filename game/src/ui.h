/* ui.h - UI rendering: text, menus, boxes, HP bars */
#ifndef UI_H
#define UI_H

#include "common.h"

/* Convert ASCII char to BG tile index */
uint8_t ui_char_to_tile(char c);

/* Print a string at BG tile position (x,y). Stops at '\0'. */
void ui_print(uint8_t x, uint8_t y, const char *str);

/* Print an unsigned number (up to 5 digits) */
void ui_print_num(uint8_t x, uint8_t y, uint16_t num);

/* Draw a bordered box (fills interior with blank tiles) */
void ui_draw_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/* Draw HP bar at position. cur/max are HP values, w = bar width in tiles */
void ui_draw_hp_bar(uint8_t x, uint8_t y, uint8_t cur, uint8_t max, uint8_t w);

/* Draw SP bar (same style, different palette) */
void ui_draw_sp_bar(uint8_t x, uint8_t y, uint8_t cur, uint8_t max, uint8_t w);

/* Clear a rectangular area with blank tiles */
void ui_clear_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/* Clear entire screen with blank tiles */
void ui_clear_screen(void);

/* Show a 2-line message box at bottom of screen. Waits for A press. */
void ui_message(const char *line1, const char *line2);

/* Show a menu and return selected index (0-based). Cancel with B = 0xFF */
uint8_t ui_menu(uint8_t x, uint8_t y, const char *const options[],
                uint8_t count);

/* Wait for any button press (debounced) */
void ui_wait_press(void);

/* Read joypad with debounce */
uint8_t ui_poll_keys(void);

/* Set BG palette attribute for a rectangular region */
void ui_set_palette_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                         uint8_t pal);

#endif
