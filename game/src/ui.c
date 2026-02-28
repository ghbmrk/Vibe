/*  ui.c  –  User-interface rendering for menus, text, HUD.
 *
 *  All rendering writes to the GBC background tile map.
 *  A custom 5x7 pixel font occupies tiles 0-44 in VRAM.
 *  UI chrome tiles (box borders, bars) occupy tiles 64-79.
 */

#include "ui.h"
#include "gfx_data.h"
#include "creature.h"
#include "skilltree.h"
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>

/* ---- Helpers ---------------------------------------------- */

/* Convert an ASCII character to its tile index */
static uint8_t char_to_tile(char c) {
    if (c >= 'A' && c <= 'Z') return (uint8_t)(c - 'A') + TILE_FONT_A;
    if (c >= 'a' && c <= 'z') return (uint8_t)(c - 'a') + TILE_FONT_A;
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0') + TILE_FONT_0;
    if (c == '!') return TILE_FONT_BANG;
    if (c == '?') return TILE_FONT_QMARK;
    if (c == '.') return TILE_FONT_DOT;
    if (c == '-') return TILE_FONT_DASH;
    if (c == '/') return TILE_FONT_SLASH;
    if (c == ':') return TILE_FONT_COLON;
    if (c == '(') return TILE_FONT_LPAREN;
    if (c == ')') return TILE_FONT_RPAREN;
    if (c == '>') return TILE_ARROW_R;
    return TILE_BLANK;   /* space / unknown */
}

/* ---- Initialisation --------------------------------------- */

void ui_init(void) {
    /* Load font tiles (0 .. FONT_TILE_COUNT-1) */
    set_bkg_data(0, FONT_TILE_COUNT, font_tiles);

    /* Load overworld tiles */
    set_bkg_data(TILE_GRASS, OW_TILE_COUNT, overworld_tiles);

    /* Load UI tiles */
    set_bkg_data(TILE_BOX_TL, UI_TILE_COUNT, ui_tiles);

    /* Load skill-tree tiles */
    set_bkg_data(TILE_NODE_LOCK, ST_TILE_COUNT, skilltree_tiles);

    /* Load player sprite tiles into OAM tile data */
    set_sprite_data(STILE_PLAYER, 4, player_sprite);

    /* Assign sprite tiles to OAM entries */
    set_sprite_tile(SPR_PLAYER_0, STILE_PLAYER);
    set_sprite_tile(SPR_PLAYER_1, STILE_PLAYER + 1);
    set_sprite_tile(SPR_PLAYER_2, STILE_PLAYER + 2);
    set_sprite_tile(SPR_PLAYER_3, STILE_PLAYER + 3);
}

/* ---- Screen clear ----------------------------------------- */

void ui_clear(void) {
    uint8_t row[MAP_W];
    uint8_t y;
    memset(row, TILE_BLANK, MAP_W);
    for (y = 0; y < MAP_H; y++) {
        set_bkg_tiles(0, y, MAP_W, 1, row);
    }
}

/* ---- Text printing ---------------------------------------- */

void ui_print(uint8_t x, uint8_t y, const char *str) {
    uint8_t buf[MAP_W];
    uint8_t i = 0;
    while (*str && i < MAP_W) {
        buf[i] = char_to_tile(*str);
        str++;
        i++;
    }
    if (i > 0) {
        set_bkg_tiles(x, y, i, 1, buf);
    }
}

void ui_print_num(uint8_t x, uint8_t y, uint16_t num) {
    char buf[6];
    int8_t i = 4;

    buf[5] = '\0';
    if (num == 0) {
        buf[4] = '0';
        i = 3;
    } else {
        while (num > 0 && i >= 0) {
            buf[i + 1] = '0'; /* will be overwritten below */
            i--;
            /* manual divmod for SDCC friendliness */
            buf[i + 2] = '0' + (char)(num % 10u);
            num /= 10u;
        }
        i++;
    }
    ui_print(x, y, &buf[i + 1]);
}

/* ---- Box drawing ------------------------------------------ */

void ui_draw_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t row[MAP_W];
    uint8_t i, iy;

    if (w > MAP_W) w = MAP_W;

    /* Top border */
    row[0] = TILE_BOX_TL;
    for (i = 1; i < w - 1; i++) row[i] = TILE_BOX_T;
    row[w - 1] = TILE_BOX_TR;
    set_bkg_tiles(x, y, w, 1, row);

    /* Middle rows */
    row[0] = TILE_BOX_L;
    for (i = 1; i < w - 1; i++) row[i] = TILE_BOX_MID;
    row[w - 1] = TILE_BOX_R;
    for (iy = 1; iy < h - 1; iy++) {
        set_bkg_tiles(x, y + iy, w, 1, row);
    }

    /* Bottom border */
    row[0] = TILE_BOX_BL;
    for (i = 1; i < w - 1; i++) row[i] = TILE_BOX_B;
    row[w - 1] = TILE_BOX_BR;
    set_bkg_tiles(x, y + h - 1, w, 1, row);
}

/* ---- HP bar ----------------------------------------------- */

void ui_draw_hp_bar(uint8_t x, uint8_t y, uint16_t current, uint16_t max) {
    uint8_t bar[10];
    uint8_t filled, i;

    if (max == 0) max = 1;
    filled = (uint8_t)((uint16_t)current * 10u / max);
    if (filled > 10) filled = 10;
    if (current > 0 && filled == 0) filled = 1;  /* show sliver */

    for (i = 0; i < 10; i++) {
        if (i < filled)     bar[i] = TILE_HP_FULL;
        else if (i == filled) bar[i] = TILE_HP_MID;
        else                bar[i] = TILE_HP_EMPTY;
    }
    set_bkg_tiles(x, y, 10, 1, bar);
}

/* ---- Skill tree visualisation ----------------------------- */

void ui_draw_skill_tree(const Creature *c, uint8_t selected) {
    const SkillTree *tree = &c->tree;
    uint8_t i, px, py;
    uint8_t node_x[MAX_SKILL_NODES];
    uint8_t node_y[MAX_SKILL_NODES];
    const SkillNode *n;
    char namebuf[NAME_LEN];

    ui_clear();

    /* Header */
    ui_print(1, 0, c->name);
    ui_print(10, 0, "SKILL TREE");
    ui_print(1, 1, "PTS:");
    ui_print_num(5, 1, c->skill_pts);

    /* Layout nodes in a simple top-down arrangement.
     * Root at top centre; children spread below. */

    /* Pass 1: assign positions based on parent relationships */
    memset(node_x, 10, MAX_SKILL_NODES);  /* default centre */
    memset(node_y, 3,  MAX_SKILL_NODES);

    /* Root */
    node_x[0] = 10;
    node_y[0] = 3;

    /* Simple layout: each node is placed relative to parent */
    {
        uint8_t col_offset = 0;
        uint8_t last_parent = 0xFF;
        for (i = 1; i < tree->count; i++) {
            n = &tree->nodes[i];
            if (n->parent != last_parent) {
                col_offset += 6;
                last_parent = n->parent;
            }
            node_x[i] = 2 + (col_offset % 18);
            node_y[i] = node_y[n->parent] + 2;
            if (node_y[i] > 13) node_y[i] = 13;
        }
    }

    /* Pass 2: draw connecting lines */
    for (i = 1; i < tree->count; i++) {
        n = &tree->nodes[i];
        if (n->parent != 0xFF && n->parent < tree->count) {
            /* Draw a vertical line segment */
            py = node_y[n->parent] + 1;
            px = node_x[n->parent];
            if (py < node_y[i]) {
                uint8_t ltile = TILE_LINE_V;
                set_bkg_tiles(px, py, 1, 1, &ltile);
            }
        }
    }

    /* Pass 3: draw nodes */
    for (i = 0; i < tree->count; i++) {
        uint8_t tile;
        n = &tree->nodes[i];
        if (i == selected) {
            tile = TILE_NODE_SEL;
        } else if (NODE_UNLOCKED(*n)) {
            tile = TILE_NODE_OPEN;
        } else {
            tile = TILE_NODE_LOCK;
        }
        set_bkg_tiles(node_x[i], node_y[i], 1, 1, &tile);
    }

    /* Bottom panel: show details of selected node */
    ui_draw_box(0, 14, 20, 4);
    n = &tree->nodes[selected];
    skilltree_skill_name(namebuf, n->element, n->category, selected & 3);
    ui_print(1, 15, namebuf);
    ui_print(12, 15, skilltree_cat_tag(n->category));

    /* Power / Cost / Level */
    ui_print(1, 16, "PWR:");
    ui_print_num(5, 16, n->power);
    ui_print(8, 16, "SP:");
    ui_print_num(11, 16, n->cost);
    ui_print(14, 16, "LV:");
    ui_print_num(17, 16, n->req_level);

    /* Status line */
    if (NODE_UNLOCKED(*n)) {
        ui_print(1, 17, "UNLOCKED");
    } else if (skilltree_can_unlock(&c->tree, selected, c->level)) {
        ui_print(1, 17, "A:UNLOCK");
    } else {
        ui_print(1, 17, "LOCKED");
    }

    /* Type name */
    ui_print(12, 17, type_names[n->element]);
}

/* ---- Title screen ----------------------------------------- */

void ui_draw_title(void) {
    ui_clear();
    ui_draw_box(2, 2, 16, 6);
    ui_print(4, 4, "CREATURE");
    ui_print(4, 5, "COLLECTOR");
    ui_print(3, 9,  "FOR CHROMATIC");
    ui_print(4, 12, "PRESS START");
    ui_print(3, 16, "2025 HOMEBREW");
}

/* ---- Starter selection ------------------------------------ */

void ui_draw_starter(uint8_t sel) {
    uint8_t i;
    ui_clear();
    ui_print(2, 0, "CHOOSE YOUR STARTER");
    ui_draw_box(1, 2, 18, 14);

    for (i = 0; i < 3; i++) {
        const SpeciesData *sp = &species_table[i];
        uint8_t y = 4 + i * 4;

        /* Arrow for selection */
        if (i == sel) ui_print(2, y, ">");

        /* Name and type */
        ui_print(4, y, sp->name);
        ui_print(13, y, type_names[sp->type]);

        /* Base stats */
        ui_print(4, y + 1, "HP:");
        ui_print_num(7, y + 1, sp->base_hp);
        ui_print(10, y + 1, "ATK:");
        ui_print_num(14, y + 1, sp->base_atk);

        ui_print(4, y + 2, "DEF:");
        ui_print_num(8, y + 2, sp->base_def);
        ui_print(11, y + 2, "SPD:");
        ui_print_num(15, y + 2, sp->base_spd);
    }
}

/* ---- In-game menu ----------------------------------------- */

void ui_draw_game_menu(uint8_t sel) {
    ui_clear();
    ui_draw_box(2, 1, 16, 14);
    ui_print(6, 2, "MENU");
    ui_print(4, 4,  "PARTY");
    ui_print(4, 6,  "SKILL TREE");
    ui_print(4, 8,  "SAVE");
    ui_print(4, 10, "CLOSE");

    /* Selection arrow */
    ui_print(3, 4 + sel * 2, ">");
}

/* ---- Party summary ---------------------------------------- */

void ui_draw_party(uint8_t sel) {
    uint8_t i, y;
    ui_clear();
    ui_print(2, 0, "PARTY");
    ui_draw_box(0, 1, 20, 16);

    for (i = 0; i < party_count; i++) {
        Creature *c = &party[i];
        y = 2 + i * 4;

        if (i == sel) ui_print(1, y, ">");
        ui_print(3, y, c->name);
        ui_print(13, y, "LV");
        ui_print_num(15, y, c->level);
        ui_print(3, y + 1, type_names[c->type]);
        ui_print(3, y + 2, "HP:");
        ui_print_num(6, y + 2, c->hp);
        ui_print(10, y + 2, "/");
        ui_print_num(11, y + 2, c->max_hp);
    }

    if (party_count == 0) {
        ui_print(3, 6, "NO CREATURES");
    }

    ui_print(3, 15, "B:BACK  A:SKILLS");
}
