/* ui.c - UI rendering */
#include "ui.h"
#include "gfx_data.h"

static uint8_t prev_keys = 0;

/* ── Character to tile mapping ─────────────────────────────── */
uint8_t ui_char_to_tile(char c) {
    if (c >= 'A' && c <= 'Z') return TILE_FONT_BASE + 1 + (c - 'A');
    if (c >= 'a' && c <= 'z') return TILE_FONT_BASE + 1 + (c - 'a');
    if (c >= '0' && c <= '9') return TILE_FONT_BASE + 27 + (c - '0');
    switch (c) {
        case ' ': return TILE_FONT_BASE;      /* 128 */
        case '!': return TILE_FONT_BASE + 37;
        case '?': return TILE_FONT_BASE + 38;
        case '.': return TILE_FONT_BASE + 39;
        case ',': return TILE_FONT_BASE + 40;
        case ':': return TILE_FONT_BASE + 41;
        case '-': return TILE_FONT_BASE + 42;
        case '/': return TILE_FONT_BASE + 43;
        case '+': return TILE_FONT_BASE + 44;
        case '=': return TILE_FONT_BASE + 45;
        case '(': return TILE_FONT_BASE + 46;
        case ')': return TILE_FONT_BASE + 47;
        case '\'': return TILE_FONT_BASE + 48;
        case '%': return TILE_FONT_BASE + 49;
        case '#': return TILE_FONT_BASE + 50;
        case '>': return TILE_FONT_BASE + 51;
        case '<': return TILE_FONT_BASE + 52;
        case '$': return TILE_FONT_BASE + 53;
        case '@': return TILE_FONT_BASE + 54;
        case '*': return TILE_FONT_BASE + 55;
        default:  return TILE_FONT_BASE;       /* space for unknown */
    }
}

/* ── Print string ──────────────────────────────────────────── */
void ui_print(uint8_t x, uint8_t y, const char *str) {
    while (*str) {
        uint8_t tile = ui_char_to_tile(*str);
        set_bkg_tile_xy(x, y, tile);
        x++;
        if (x >= SCREEN_W) break;
        str++;
    }
}

/* ── Print number ──────────────────────────────────────────── */
void ui_print_num(uint8_t x, uint8_t y, uint16_t num) {
    char buf[6];
    int8_t i = 4;
    buf[5] = '\0';

    if (num == 0) {
        buf[4] = '0';
        i = 3;
    } else {
        while (num > 0 && i >= 0) {
            buf[i + 1] = '0' + (num % 10);
            num /= 10;
            if (num > 0) i--;
        }
    }
    ui_print(x, y, &buf[i + 1]);
}

/* ── Draw box ──────────────────────────────────────────────── */
void ui_draw_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t i, j;
    uint8_t x2 = x + w - 1;
    uint8_t y2 = y + h - 1;

    /* Corners */
    set_bkg_tile_xy(x,  y,  TILE_BOX_TL);
    set_bkg_tile_xy(x2, y,  TILE_BOX_TR);
    set_bkg_tile_xy(x,  y2, TILE_BOX_BL);
    set_bkg_tile_xy(x2, y2, TILE_BOX_BR);

    /* Horizontal edges */
    for (i = x + 1; i < x2; i++) {
        set_bkg_tile_xy(i, y,  TILE_BOX_H);
        set_bkg_tile_xy(i, y2, TILE_BOX_H);
    }

    /* Vertical edges */
    for (j = y + 1; j < y2; j++) {
        set_bkg_tile_xy(x,  j, TILE_BOX_V);
        set_bkg_tile_xy(x2, j, TILE_BOX_V);
    }

    /* Fill interior */
    for (j = y + 1; j < y2; j++) {
        for (i = x + 1; i < x2; i++) {
            set_bkg_tile_xy(i, j, TILE_FONT_BASE); /* space */
        }
    }
}

/* ── HP bar ────────────────────────────────────────────────── */
void ui_draw_hp_bar(uint8_t x, uint8_t y, uint8_t cur, uint8_t max,
                    uint8_t w) {
    uint8_t filled, i;
    if (max == 0) max = 1;
    filled = (uint8_t)(((uint16_t)cur * (uint16_t)w) / (uint16_t)max);
    if (cur > 0 && filled == 0) filled = 1;

    for (i = 0; i < w; i++) {
        set_bkg_tile_xy(x + i, y, (i < filled) ? TILE_HP_FULL : TILE_HP_EMPTY);
    }
}

void ui_draw_sp_bar(uint8_t x, uint8_t y, uint8_t cur, uint8_t max,
                    uint8_t w) {
    /* Same visual as HP bar for now */
    ui_draw_hp_bar(x, y, cur, max, w);
}

/* ── Clear rect ────────────────────────────────────────────── */
void ui_clear_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t i, j;
    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            set_bkg_tile_xy(x + i, y + j, TILE_FONT_BASE);
        }
    }
}

void ui_clear_screen(void) {
    ui_clear_rect(0, 0, SCREEN_W, SCREEN_H);
}

/* ── Debounced key polling ─────────────────────────────────── */
uint8_t ui_poll_keys(void) {
    uint8_t keys = joypad();
    uint8_t pressed = keys & ~prev_keys;
    prev_keys = keys;
    return pressed;
}

/* ── Wait for button press ─────────────────────────────────── */
void ui_wait_press(void) {
    /* Wait for release first */
    while (joypad()) {
        wait_vbl_done();
    }
    /* Wait for press */
    while (!joypad()) {
        wait_vbl_done();
    }
    /* Wait for release */
    while (joypad()) {
        wait_vbl_done();
    }
}

/* ── Message box ───────────────────────────────────────────── */
void ui_message(const char *line1, const char *line2) {
    ui_draw_box(0, 13, 20, 5);
    ui_print(1, 14, line1);
    if (line2) {
        ui_print(1, 15, line2);
    }
    ui_print(17, 16, "...");
    ui_wait_press();
}

/* ── Menu ──────────────────────────────────────────────────── */
uint8_t ui_menu(uint8_t x, uint8_t y, const char *const options[],
                uint8_t count) {
    uint8_t sel = 0;
    uint8_t i;
    uint8_t pressed;

    /* Draw options */
    for (i = 0; i < count; i++) {
        ui_print(x + 2, y + i, options[i]);
    }

    for (;;) {
        /* Draw cursor */
        for (i = 0; i < count; i++) {
            set_bkg_tile_xy(x, y + i,
                            (i == sel) ? TILE_CURSOR : TILE_FONT_BASE);
        }

        wait_vbl_done();
        pressed = ui_poll_keys();

        if (pressed & J_UP) {
            if (sel > 0) sel--;
        }
        if (pressed & J_DOWN) {
            if (sel < count - 1) sel++;
        }
        if (pressed & J_A) {
            return sel;
        }
        if (pressed & J_B) {
            return 0xFF; /* cancel */
        }
    }
}

/* ── Set palette for a region ──────────────────────────────── */
void ui_set_palette_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                         uint8_t pal) {
    uint8_t i, j;
    uint8_t attr_row[20];

    for (i = 0; i < w && i < 20; i++) {
        attr_row[i] = pal;
    }

    VBK_REG = 1;
    for (j = 0; j < h; j++) {
        set_bkg_tiles(x, y + j, w, 1, attr_row);
    }
    VBK_REG = 0;
}
