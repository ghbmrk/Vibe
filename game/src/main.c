#include "common.h"
#include "rng.h"
#include "gfx_data.h"
#include "creature.h"
#include "ui.h"
#include "battle.h"
#include "world.h"
#include "save.h"
#include <gb/gb.h>
#include <gb/cgb.h>

/* =========================================================
   GLOBAL VARIABLES
   ========================================================= */
SaveData save;
uint8_t game_state;
uint8_t jpad;
uint8_t jpad_prev;
uint8_t frame_count;

/* =========================================================
   TITLE SCREEN
   ========================================================= */
static void title_screen(void) {
    ui_clear_screen();

    /* Set palette for title */
    ui_set_area_palette(0, 0, 20, 18, 0);

    /* Title text */
    ui_print(3, 3, "CREATURE");
    ui_print(5, 5, "QUEST");

    ui_print(3, 10, "A: New Game");
    ui_print(3, 12, "B: Continue");

    ui_print(2, 16, "2025 Vibe Games");

    /* Wait for input */
    while (1) {
        wait_vbl_done();
        jpad = joypad();

        if (J_PRESSED(J_A)) {
            /* New game */
            save_init();
            game_state = STATE_INTRO;
            break;
        }
        if (J_PRESSED(J_B)) {
            /* Load save */
            if (save_load()) {
                game_state = STATE_OVERWORLD;
            } else {
                ui_show_message("No save found!", "");
                ui_wait_button();
                ui_clear_box(0, 14, 20, 4);
            }
            break;
        }

        jpad_prev = jpad;
    }
}

/* =========================================================
   INTRO / STARTER SELECTION
   ========================================================= */
static void intro_sequence(void) {
    const char *starters[] = {"Embrix", "Tidalin", "Thornyx"};
    uint8_t choice;

    ui_clear_screen();
    ui_set_area_palette(0, 0, 20, 18, 0);

    ui_show_message("Welcome to the", "world of creatures!");
    ui_wait_button();

    ui_show_message("Choose your", "first partner!");
    ui_wait_button();

    ui_clear_screen();
    ui_print(2, 1, "Choose a creature:");

    /* Show starter info */
    ui_print(3, 4, "Embrix");
    ui_print(5, 5, "Fire type");

    ui_print(3, 7, "Tidalin");
    ui_print(5, 8, "Water type");

    ui_print(3, 10, "Thornyx");
    ui_print(5, 11, "Leaf type");

    ui_draw_box(0, 13, 20, 5);
    choice = ui_menu(1, 14, starters, 3);
    if (choice == 0xFF) choice = 0; /* default to Embrix */

    /* Initialize starter */
    save.party_count = 1;
    creature_init(&save.party[0], choice, 5);

    ui_show_message("You chose", species_db[choice].name);
    ui_wait_button();

    ui_show_message("Good luck on", "your quest!");
    ui_wait_button();

    game_state = STATE_OVERWORLD;
}

/* =========================================================
   PARTY MENU
   ========================================================= */
static void party_menu(void) {
    uint8_t i;
    uint8_t choice;
    const char *menu_items[] = {"Party", "Items", "Save", "Back"};

    ui_clear_screen();
    ui_set_area_palette(0, 0, 20, 18, 0);

    ui_print(1, 0, "Menu");
    ui_print(1, 1, "Zone:");
    ui_print(7, 1, zone_db[save.current_zone].name);
    ui_print(1, 2, "Gold:");
    ui_print_num(7, 2, save.gold, 5);
    ui_print(1, 3, "Badges:");
    ui_print_num(9, 3, save.badges, 1);

    ui_draw_box(0, 12, 20, 6);
    choice = ui_menu(1, 13, menu_items, 4);

    switch (choice) {
        case 0: { /* Party */
            ui_clear_screen();
            ui_print(1, 0, "Party:");
            for (i = 0; i < save.party_count; i++) {
                Creature *c = &save.party[i];
                uint8_t row = 2 + i * 4;
                ui_print(1, row, species_db[c->species].name);
                ui_print(1, row + 1, "Lv");
                ui_print_num(3, row + 1, c->level, 2);
                ui_print(7, row + 1, "HP");
                ui_print_num(9, row + 1, c->hp, 3);
                ui_print(12, row + 1, "/");
                ui_print_num(13, row + 1, c->max_hp, 3);
                ui_draw_hp_bar(1, row + 2, c->hp, c->max_hp);
            }
            ui_wait_button();
            break;
        }
        case 1: { /* Items */
            const char *item_names[] = {"Potion", "Elixir", "Revive", "Antidote", "SmokeBall"};
            ui_clear_screen();
            ui_print(1, 0, "Items:");
            for (i = 1; i < NUM_ITEM_TYPES; i++) {
                ui_print(2, 1 + i, item_names[i - 1]);
                ui_print(14, 1 + i, "x");
                ui_print_num(15, 1 + i, save.items[i], 2);
            }
            ui_wait_button();
            break;
        }
        case 2: { /* Save */
            save_write();
            ui_show_message("Game saved!", "");
            ui_wait_button();
            break;
        }
        default:
            break;
    }
}

/* =========================================================
   OVERWORLD STATE
   ========================================================= */
static void overworld_loop(void) {
    uint8_t encounter;

    /* Generate and draw current zone */
    world_generate_zone();
    ui_fade_out();
    world_draw();
    ui_fade_in();

    while (game_state == STATE_OVERWORLD) {
        wait_vbl_done();
        frame_count++;

        jpad = joypad();
        encounter = 0;

        /* Movement */
        if (J_PRESSED(J_UP))    encounter = world_move_player(DIR_UP);
        if (J_PRESSED(J_DOWN))  encounter = world_move_player(DIR_DOWN);
        if (J_PRESSED(J_LEFT))  encounter = world_move_player(DIR_LEFT);
        if (J_PRESSED(J_RIGHT)) encounter = world_move_player(DIR_RIGHT);

        /* Random encounter */
        if (encounter) {
            Creature wild;
            const ZoneData *zd = &zone_db[save.current_zone];
            uint8_t species = zd->wild_species[rng_range(0, 3)];
            uint8_t level = rng_range(zd->wild_min_level, zd->wild_max_level);

            creature_init(&wild, species, level);
            battle_wild(&wild);

            /* Capture chance: if player won, small chance to add to party */
            if (wild.hp == 0 && save.party_count < MAX_PARTY) {
                if (rng_range(1, 100) <= 30) {
                    ui_show_message("Creature wants", "to join you!");
                    ui_wait_button();
                    ui_show_message("Accept?", "");
                    if (ui_yes_no(10, 16)) {
                        creature_init(&save.party[save.party_count], species, level);
                        save.party_count++;
                        ui_show_message(species_db[species].name, "joined!");
                        ui_wait_button();
                    }
                }
            }

            ui_fade_out();
            world_draw();
            ui_fade_in();
        }

        /* Menu */
        if (J_PRESSED(J_START)) {
            party_menu();
            world_draw();
        }

        /* Water animation */
        world_update();

        jpad_prev = jpad;
    }
}

/* =========================================================
   MAIN ENTRY POINT
   ========================================================= */
void main(void) {
    /* Ensure CGB mode */
    if (_cpu != CGB_TYPE) {
        /* Still works on DMG but no color */
    }

    /* Initialize subsystems */
    rng_init();
    gfx_init();

    /* DMG palette fallback */
    BGP_REG = 0xE4;
    OBP0_REG = 0xD2;
    OBP1_REG = 0xD2;

    game_state = STATE_TITLE;

    /* Main game state machine */
    while (1) {
        switch (game_state) {
            case STATE_TITLE:
                title_screen();
                break;

            case STATE_INTRO:
                intro_sequence();
                break;

            case STATE_OVERWORLD:
                overworld_loop();
                break;

            default:
                game_state = STATE_TITLE;
                break;
        }
    }
}
