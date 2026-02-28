/*  main.c  –  Creature Collector for ModRetro Chromatic (GBC)
 *
 *  Entry point and top-level state machine.
 *
 *  Build with GBDK-2020:
 *      make          (uses Makefile in game/ directory)
 *
 *  States:
 *      TITLE      →  START pressed  →  STARTER / continue
 *      STARTER    →  chose creature  →  OVERWORLD
 *      OVERWORLD  →  encounter       →  BATTLE
 *                 →  START pressed   →  MENU
 *      BATTLE     →  finished        →  OVERWORLD
 *      MENU       →  closed          →  OVERWORLD
 *                 →  skill tree      →  SKILLTREE
 *      SKILLTREE  →  B pressed       →  back to caller
 */

#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>

#include "common.h"
#include "rng.h"
#include "creature.h"
#include "skilltree.h"
#include "battle.h"
#include "world.h"
#include "ui.h"
#include "save.h"
#include "gfx_data.h"

/* ======== Global game state ================================ */

uint8_t  game_state;
uint8_t  party_count;
Creature party[MAX_PARTY];
uint8_t  current_map;
uint8_t  player_x;
uint8_t  player_y;
uint8_t  player_dir;
uint16_t battles_won;
uint8_t  total_catches;
uint8_t  jpad;
uint8_t  jpad_prev;

/* ---- Local state for menus / sub-screens ----------------- */

static uint8_t menu_sel;
static uint8_t st_creature_idx;   /* creature whose tree is shown */
static uint8_t st_node_sel;       /* selected skill-tree node     */
static uint8_t prev_state;        /* state to return to           */

/* Scratch creature for wild encounters */
static Creature wild;

/* ---- Entropy helper: sample DIV register while waiting ---- */
static uint16_t gather_entropy(void) {
    uint16_t e = 0;
    uint8_t i;
    for (i = 0; i < 16; i++) {
        e = (e << 1) | (DIV_REG & 1u);
        /* Busy wait a tiny bit for jitter */
        __asm__("nop");
        __asm__("nop");
    }
    return e;
}

/* ---- GBC palette setup ------------------------------------ */
static void setup_palettes(void) {
    set_bkg_palette(0, 8, (const uint16_t *)bg_palettes);
    set_sprite_palette(0, 8, (const uint16_t *)spr_palettes);

    /* Assign palette 0 to all sprite entries used by the player */
    set_sprite_prop(SPR_PLAYER_0, 0);
    set_sprite_prop(SPR_PLAYER_1, 0);
    set_sprite_prop(SPR_PLAYER_2, 0);
    set_sprite_prop(SPR_PLAYER_3, 0);
}

/* ---- Wild encounter generation ----------------------------- */
/* Species pools and level ranges are defined in world.c;
 * we reproduce the extern data we need here via simple look-ups. */

static void generate_wild(void) {
    uint8_t species;
    uint8_t level;
    uint16_t seed;

    /* Pick species from a small pool based on current map */
    switch (current_map) {
        case 0:  species = rng_range(0, 2); break;  /* starters */
        case 1:  species = rng_range(0, 3); break;  /* + Zappix */
        default: species = rng_range(3, 5); break;  /* rares    */
    }

    /* Level range per map */
    switch (current_map) {
        case 0:  level = rng_range(2, 4);  break;
        case 1:  level = rng_range(3, 7);  break;
        default: level = rng_range(6, 12); break;
    }

    seed = rng_next();
    creature_init(&wild, species, level, seed);
}

/* ============================================================
 *  MAIN
 * ============================================================ */

void main(void) {

    /* ---- Hardware init ------------------------------------- */
    DISPLAY_OFF;

    /* Use double-speed CPU on GBC for smoother updates */
    if (_cpu == CGB_TYPE) {
        cpu_fast();
    }

    SPRITES_8x8;
    SHOW_BKG;
    SHOW_SPRITES;

    /* Reset scroll */
    SCX_REG = 0;
    SCY_REG = 0;

    /* Load tiles, sprites, palettes */
    ui_init();
    setup_palettes();

    /* Seed RNG from the hardware DIV register */
    rng_seed(gather_entropy());

    /* Default player position */
    player_x   = 9;
    player_y   = 8;
    player_dir = DIR_DOWN;

    DISPLAY_ON;

    /* ---- Start at title screen ----------------------------- */
    game_state = STATE_TITLE;
    menu_sel   = 0;
    st_creature_idx = 0;
    st_node_sel = 0;

    /* ======================================================= */
    /*  MAIN LOOP                                               */
    /* ======================================================= */

    while (1) {
        /* Sample input */
        jpad_prev = jpad;
        jpad = joypad();

        switch (game_state) {

        /* ---- TITLE SCREEN --------------------------------- */
        case STATE_TITLE:
            ui_draw_title();

            if (PRESSED(J_START)) {
                /* Try to load a save; if none, go to starter select */
                if (save_exists() && load_game()) {
                    world_load(current_map);
                    game_state = STATE_OVERWORLD;
                } else {
                    game_state = STATE_STARTER;
                    menu_sel = 0;
                }
            }
            break;

        /* ---- STARTER SELECTION ----------------------------- */
        case STATE_STARTER:
            ui_draw_starter(menu_sel);

            if (PRESSED(J_UP)   && menu_sel > 0) menu_sel--;
            if (PRESSED(J_DOWN) && menu_sel < 2) menu_sel++;

            if (PRESSED(J_A)) {
                /* Create starter creature at level 5 */
                uint16_t seed = rng_next();
                party_count = 1;
                creature_init(&party[0], menu_sel, 5, seed);

                /* Start in the village */
                current_map = 0;
                player_x = 9;
                player_y = 8;
                battles_won = 0;
                total_catches = 0;

                world_load(current_map);
                game_state = STATE_OVERWORLD;
            }
            break;

        /* ---- OVERWORLD ------------------------------------- */
        case STATE_OVERWORLD:
            world_show_player();

            if (world_update()) {
                /* Wild encounter triggered! */
                generate_wild();
                world_hide_player();
                battle_start(&wild);
                game_state = STATE_BATTLE;
                break;
            }

            /* Open menu with START */
            if (PRESSED(J_START)) {
                world_hide_player();
                menu_sel = 0;
                game_state = STATE_MENU;
                break;
            }

            world_render();
            break;

        /* ---- BATTLE ---------------------------------------- */
        case STATE_BATTLE:
            /* The battle may temporarily switch to SKILLTREE
             * (via the "SKILLS" menu option).  When we come back
             * from SKILLTREE, battle resumes because bstate
             * is still BSTATE_PLAYER_MENU.  We handle that by
             * checking if game_state was changed by battle_update. */
            if (battle_update()) {
                /* Battle over – return to overworld */
                game_state = STATE_OVERWORLD;
                break;
            }

            /* If battle_update set game_state to SKILLTREE, honour it */
            if (game_state == STATE_SKILLTREE) {
                st_creature_idx = 0;
                st_node_sel = 0;
                prev_state = STATE_BATTLE;
                break;
            }

            battle_render();
            break;

        /* ---- GAME MENU ------------------------------------- */
        case STATE_MENU:
            ui_draw_game_menu(menu_sel);

            if (PRESSED(J_UP)   && menu_sel > 0) menu_sel--;
            if (PRESSED(J_DOWN) && menu_sel < 3) menu_sel++;

            if (PRESSED(J_B)) {
                game_state = STATE_OVERWORLD;
                break;
            }

            if (PRESSED(J_A)) {
                switch (menu_sel) {
                    case 0: /* PARTY */
                        menu_sel = 0;
                        /* Re-use MENU state but render party */
                        ui_draw_party(menu_sel);
                        /* Simple sub-loop for party screen */
                        {
                            uint8_t in_party = 1;
                            while (in_party) {
                                wait_vbl_done();
                                jpad_prev = jpad;
                                jpad = joypad();
                                if (PRESSED(J_UP) && menu_sel > 0) {
                                    menu_sel--;
                                    ui_draw_party(menu_sel);
                                }
                                if (PRESSED(J_DOWN) &&
                                    menu_sel < party_count - 1) {
                                    menu_sel++;
                                    ui_draw_party(menu_sel);
                                }
                                if (PRESSED(J_A) && party_count > 0) {
                                    /* Open skill tree for selected creature */
                                    st_creature_idx = menu_sel;
                                    st_node_sel = 0;
                                    prev_state = STATE_MENU;
                                    game_state = STATE_SKILLTREE;
                                    in_party = 0;
                                }
                                if (PRESSED(J_B)) {
                                    menu_sel = 0;
                                    in_party = 0;
                                }
                            }
                        }
                        break;
                    case 1: /* SKILL TREE (lead creature) */
                        st_creature_idx = 0;
                        st_node_sel = 0;
                        prev_state = STATE_MENU;
                        game_state = STATE_SKILLTREE;
                        break;
                    case 2: /* SAVE */
                        save_game();
                        ui_clear();
                        ui_draw_box(4, 7, 12, 4);
                        ui_print(6, 8, "SAVED!");
                        /* Brief pause */
                        {
                            uint8_t t;
                            for (t = 0; t < 60; t++) wait_vbl_done();
                        }
                        break;
                    case 3: /* CLOSE */
                        game_state = STATE_OVERWORLD;
                        break;
                }
            }
            break;

        /* ---- SKILL TREE VIEWER ----------------------------- */
        case STATE_SKILLTREE: {
            Creature *c = &party[st_creature_idx];

            ui_draw_skill_tree(c, st_node_sel);

            /* Navigate nodes */
            if (PRESSED(J_UP)   && st_node_sel > 0)
                st_node_sel--;
            if (PRESSED(J_DOWN) && st_node_sel < c->tree.count - 1)
                st_node_sel++;

            /* Unlock node */
            if (PRESSED(J_A)) {
                if (c->skill_pts > 0 &&
                    skilltree_can_unlock(&c->tree, st_node_sel, c->level)) {
                    skilltree_unlock(&c->tree, st_node_sel);
                    c->skill_pts--;
                }
            }

            /* Exit */
            if (PRESSED(J_B)) {
                game_state = prev_state;
            }
            break;
        }

        } /* end switch(game_state) */

        /* Wait for VBlank (≈60 Hz frame sync) */
        wait_vbl_done();
    }
}
