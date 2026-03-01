#include "ui.h"
#include "gfx_data.h"
#include <gb/gb.h>
#include <gb/cgb.h>

/* =========================================================
   TEXT RENDERING
   Maps ASCII to our font tile indices
   ========================================================= */
static uint8_t char_to_tile(char c) {
    if (c >= 'A' && c <= 'Z') return TILE_FONT_START + (c - 'A');
    if (c >= 'a' && c <= 'z') return TILE_FONT_START + 26 + (c - 'a');
    if (c >= '0' && c <= '9') return TILE_FONT_START + 52 + (c - '0');
    switch (c) {
        case ' ': return TILE_FONT_START + 62;
        case '!': return TILE_FONT_START + 63;
        case '?': return TILE_FONT_START + 64;
        case '.': return TILE_FONT_START + 65;
        case ',': return TILE_FONT_START + 66;
        case '-': return TILE_FONT_START + 67;
        case ':': return TILE_FONT_START + 68;
        case '/': return TILE_FONT_START + 69;
        default:  return TILE_FONT_START + 62; /* space for unknown */
    }
}

void ui_print(uint8_t x, uint8_t y, const char *str) {
    uint8_t tiles[20];
    uint8_t i = 0;
    while (str[i] && i < 20) {
        tiles[i] = char_to_tile(str[i]);
        i++;
    }
    if (i > 0) {
        set_bkg_tiles(x, y, i, 1, tiles);
    }
}

void ui_print_num(uint8_t x, uint8_t y, uint16_t num, uint8_t digits) {
    uint8_t buf[5];
    uint8_t tiles[5];
    int8_t i;

    for (i = digits - 1; i >= 0; i--) {
        buf[i] = num % 10;
        num /= 10;
    }
    for (i = 0; i < digits; i++) {
        tiles[i] = TILE_FONT_START + 52 + buf[i];
    }
    set_bkg_tiles(x, y, digits, 1, tiles);
}

/* =========================================================
   BOX DRAWING
   Uses UI tiles for borders
   ========================================================= */
void ui_draw_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t row[20];
    uint8_t i;

    /* Top row */
    row[0] = TILE_UI_START + 0; /* TL corner */
    for (i = 1; i < w - 1; i++) row[i] = TILE_UI_START + 1; /* top */
    row[w - 1] = TILE_UI_START + 2; /* TR corner */
    set_bkg_tiles(x, y, w, 1, row);

    /* Middle rows */
    row[0] = TILE_UI_START + 3; /* left */
    for (i = 1; i < w - 1; i++) row[i] = TILE_UI_START + 4; /* fill */
    row[w - 1] = TILE_UI_START + 5; /* right */
    for (i = 1; i < h - 1; i++) {
        set_bkg_tiles(x, y + i, w, 1, row);
    }

    /* Bottom row */
    row[0] = TILE_UI_START + 6; /* BL corner */
    for (i = 1; i < w - 1; i++) row[i] = TILE_UI_START + 7; /* bottom */
    row[w - 1] = TILE_UI_START + 8; /* BR corner */
    set_bkg_tiles(x, y + h - 1, w, 1, row);
}

void ui_clear_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t row[20];
    uint8_t i, j;
    for (i = 0; i < w && i < 20; i++) row[i] = TILE_BLANK;
    for (j = 0; j < h; j++) {
        set_bkg_tiles(x, y + j, w, 1, row);
    }
}

void ui_clear_screen(void) {
    uint8_t row[20];
    uint8_t j;
    memset(row, TILE_BLANK, 20);
    for (j = 0; j < 18; j++) {
        set_bkg_tiles(0, j, 20, 1, row);
    }
}

/* =========================================================
   MESSAGE BOX
   Two-line message at bottom of screen
   ========================================================= */
void ui_show_message(const char *line1, const char *line2) {
    ui_draw_box(0, 14, 20, 4);
    if (line1) ui_print(1, 15, line1);
    if (line2) ui_print(1, 16, line2);
}

void ui_wait_button(void) {
    /* Wait for any button currently held to be released */
    while (joypad()) {
        wait_vbl_done();
    }
    /* Now wait for new press */
    while (!joypad()) {
        wait_vbl_done();
    }
    /* Wait for release */
    while (joypad()) {
        wait_vbl_done();
    }
}

/* =========================================================
   HP BAR
   Draws a bar using tiles: [==== ] style
   Width: 8 tiles. Uses HP full/half/empty tiles.
   ========================================================= */
void ui_draw_hp_bar(uint8_t x, uint8_t y, uint16_t hp, uint16_t max_hp) {
    uint8_t bar[8];
    uint8_t filled;
    uint8_t i;

    if (max_hp == 0) max_hp = 1;
    filled = (uint8_t)((hp * 8) / max_hp);
    if (hp > 0 && filled == 0) filled = 1;

    for (i = 0; i < 8; i++) {
        if (i < filled) {
            bar[i] = TILE_UI_START + 9;  /* full */
        } else {
            bar[i] = TILE_UI_START + 11; /* empty */
        }
    }
    set_bkg_tiles(x, y, 8, 1, bar);
}

/* =========================================================
   MENU SYSTEM
   Draws menu items with a cursor, handles input
   Returns selected index or 0xFF if B pressed
   ========================================================= */
uint8_t ui_menu(uint8_t x, uint8_t y, const char **items, uint8_t count) {
    uint8_t sel = 0;
    uint8_t cursor_tile;
    uint8_t blank_tile;
    uint8_t i;
    uint8_t prev_pad = 0;
    uint8_t cur_pad;

    cursor_tile = TILE_UI_START + 12;
    blank_tile = TILE_UI_START + 4;

    /* Draw items */
    for (i = 0; i < count; i++) {
        ui_print(x + 1, y + i, items[i]);
    }

    while (1) {
        /* Draw cursor */
        for (i = 0; i < count; i++) {
            uint8_t t = (i == sel) ? cursor_tile : blank_tile;
            set_bkg_tiles(x, y + i, 1, 1, &t);
        }

        wait_vbl_done();
        cur_pad = joypad();

        if ((cur_pad & J_UP) && !(prev_pad & J_UP)) {
            if (sel > 0) sel--;
            else sel = count - 1;
        }
        if ((cur_pad & J_DOWN) && !(prev_pad & J_DOWN)) {
            if (sel < count - 1) sel++;
            else sel = 0;
        }
        if ((cur_pad & J_A) && !(prev_pad & J_A)) {
            return sel;
        }
        if ((cur_pad & J_B) && !(prev_pad & J_B)) {
            return 0xFF;
        }

        prev_pad = cur_pad;
    }
}

uint8_t ui_yes_no(uint8_t x, uint8_t y) {
    const char *items[] = {"Yes", "No"};
    uint8_t r = ui_menu(x, y, items, 2);
    return (r == 0) ? 1 : 0;
}

/* =========================================================
   SCREEN TRANSITIONS (CGB palette fading)
   ========================================================= */
void ui_fade_out(void) {
    uint8_t i;
    for (i = 0; i < 4; i++) {
        uint8_t val = (i * 64);
        BGP_REG = val | (val << 2);
        OBP0_REG = val | (val << 2);
        wait_vbl_done();
        wait_vbl_done();
        wait_vbl_done();
        wait_vbl_done();
    }
}

void ui_fade_in(void) {
    uint8_t i;
    for (i = 4; i > 0; i--) {
        uint8_t val = ((i - 1) * 64);
        BGP_REG = 0xE4;
        OBP0_REG = 0xE4;
        if (i > 1) {
            BGP_REG = val;
            OBP0_REG = val;
        }
        wait_vbl_done();
        wait_vbl_done();
        wait_vbl_done();
        wait_vbl_done();
    }
    BGP_REG = 0xE4;
    OBP0_REG = 0xE4;
}

void ui_set_area_palette(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pal) {
    uint8_t pal_row[20];
    uint8_t i, j;
    for (i = 0; i < w && i < 20; i++) pal_row[i] = pal;
    VBK_REG = 1; /* switch to attribute map */
    for (j = 0; j < h; j++) {
        set_bkg_tiles(x, y + j, w, 1, pal_row);
    }
    VBK_REG = 0; /* back to tile map */
}
