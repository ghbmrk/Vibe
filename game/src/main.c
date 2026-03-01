/* main.c - IMPERIAL HUNT - Game Boy Color
   A Warhammer 40K Space Marine creature collector RPG */
#include "common.h"
#include "rng.h"
#include "gfx_data.h"
#include "ui.h"
#include "creature.h"
#include "character.h"
#include "skilltree.h"
#include "battle.h"
#include "world.h"
#include "zone.h"
#include "shop.h"
#include "save.h"

/* ── Global game data ──────────────────────────────────────── */
GameData game;

/* ── Forward declarations ──────────────────────────────────── */
static void state_title(void);
static void state_starter(void);
static void state_overworld(void);
static void state_battle_encounter(uint8_t is_boss);
static void state_shop(void);
static void state_skilltree_char(void);
static void state_skilltree_creature(void);
static void state_gameover(void);
static void state_menu(void);
static void enter_zone(uint8_t zone_num);
static void spawn_wild_creature(Creature *wild);
static void spawn_boss_creature(Creature *boss);

/* ── Initialization ────────────────────────────────────────── */
static void init_hardware(void) {
    /* CGB mode */
    if (_cpu == CGB_TYPE) {
        cpu_fast();
    }

    DISPLAY_OFF;

    /* Load all graphics */
    gfx_load_font();
    gfx_load_terrain();
    gfx_load_ui_tiles();
    gfx_load_player_sprite();

    /* Set palettes */
    gfx_set_overworld_palettes();

    /* Set up display */
    SHOW_BKG;
    SHOW_SPRITES;
    SPRITES_8x8;

    DISPLAY_ON;
}

/* ── Title screen ──────────────────────────────────────────── */
static void state_title(void) {
    ui_clear_screen();
    ui_set_palette_rect(0, 0, 20, 18, PAL_UI);

    /* Title text */
    ui_print(3, 3, "IMPERIAL HUNT");
    ui_print(2, 6, "A SPACE MARINE'S");
    ui_print(3, 7, "CREATURE QUEST");

    ui_print(4, 11, "PRESS START");

    ui_print(2, 15, "NEW GAME");
    ui_print(2, 16, "CONTINUE");

    /* Check for save */
    {
        uint8_t has_save = 0;
        uint8_t pressed;

        /* Test if save exists */
        ENABLE_RAM;
        if (((uint8_t *)0xA000)[0] == 0x49 &&
            ((uint8_t *)0xA000)[1] == 0x48) {
            has_save = 1;
        }
        DISABLE_RAM;

        if (!has_save) {
            ui_print(2, 16, "          "); /* hide continue */
        }

        /* Wait for START */
        for (;;) {
            wait_vbl_done();
            pressed = ui_poll_keys();

            if (pressed & J_START) {
                if (has_save && load_game()) {
                    /* Restore into zone */
                    enter_zone(game.zone.zone_num);
                    game.state = ST_OVERWORLD;
                } else {
                    game.state = ST_STARTER;
                }
                return;
            }
        }
    }
}

/* ── Starter selection ─────────────────────────────────────── */
static void state_starter(void) {
    uint8_t sel = 0;
    uint8_t pressed;

    memset(&game, 0, sizeof(GameData));
    character_init(&game.player);

    /* Seed RNG from a simple counter */
    rng_seed(DIV_REG | ((uint16_t)DIV_REG << 8));

    ui_clear_screen();
    ui_set_palette_rect(0, 0, 20, 18, PAL_UI);

    ui_print(1, 0, "CHOOSE YOUR PARTNER");
    ui_print(1, 2, "THE EMPEROR GRANTS");
    ui_print(1, 3, "YOU A COMPANION.");

    /* Show 3 starters */
    ui_print(3, 6,  "EMBERON");
    ui_print(3, 7,  "FIRE TYPE");

    ui_print(3, 9,  "TIDALIN");
    ui_print(3, 10, "WATER TYPE");

    ui_print(3, 12, "TERRAVOLT");
    ui_print(3, 13, "EARTH TYPE");

    for (;;) {
        /* Draw cursor */
        set_bkg_tile_xy(1, 6,  (sel == 0) ? TILE_CURSOR : TILE_FONT_BASE);
        set_bkg_tile_xy(1, 9,  (sel == 1) ? TILE_CURSOR : TILE_FONT_BASE);
        set_bkg_tile_xy(1, 12, (sel == 2) ? TILE_CURSOR : TILE_FONT_BASE);

        wait_vbl_done();
        pressed = ui_poll_keys();

        if (pressed & J_UP) {
            if (sel > 0) sel--;
        }
        if (pressed & J_DOWN) {
            if (sel < 2) sel++;
        }
        if (pressed & J_A) {
            /* Create starter creature */
            creature_create(&game.creature, sel, 5);

            ui_clear_screen();
            ui_print(1, 8, "YOU CHOSE");
            ui_print(1, 9, species_names[sel]);
            ui_print(1, 10, "!");
            ui_print(1, 12, "FOR THE EMPEROR!");
            ui_wait_press();

            /* Enter zone 0 */
            enter_zone(0);
            game.state = ST_OVERWORLD;
            return;
        }
    }
}

/* ── Enter a zone ──────────────────────────────────────────── */
static void enter_zone(uint8_t zone_num) {
    zone_generate(&game.zone, zone_num);
    zone_generate_gym(&game.zone);
    game.px = game.zone.entry_x;
    game.py = game.zone.entry_y;
    game.dir = DIR_DOWN;
    game.in_gym = 0;
}

/* ── Spawn a wild creature for the current zone ────────────── */
static void spawn_wild_creature(Creature *wild) {
    uint8_t species;
    uint8_t level;

    /* Species based on zone theme with some randomness */
    if (rng_chance(60)) {
        species = game.zone.theme; /* theme-matching species */
    } else {
        species = rng_range(0, SP_COUNT - 1);
    }

    /* Level: base +/- 2 */
    level = game.zone.base_level;
    if (level > 2) {
        level = rng_range(level - 2, level + 2);
    }
    if (level < 1) level = 1;
    if (level > MAX_LEVEL) level = MAX_LEVEL;

    creature_create(wild, species, level);

    /* Higher-zone creatures may know extra moves */
    if (game.zone.zone_num >= 2 && wild->num_moves < MAX_MOVES) {
        /* Add a mid-tier move of their type */
        uint8_t mid_move;
        if (wild->type < ELEM_NORMAL) {
            mid_move = 7 + wild->type; /* BLAZE..RADIANCE */
        } else {
            mid_move = MOVE_STRIKE;
        }
        wild->moves[wild->num_moves++] = mid_move;
    }
    if (game.zone.zone_num >= 5 && wild->num_moves < MAX_MOVES) {
        uint8_t strong_move;
        if (wild->type < ELEM_NORMAL) {
            strong_move = 13 + wild->type; /* INFERNO..NOVA */
        } else {
            strong_move = MOVE_SLAM;
        }
        wild->moves[wild->num_moves++] = strong_move;
    }
}

/* ── Spawn boss creature ───────────────────────────────────── */
static void spawn_boss_creature(Creature *boss) {
    uint8_t species = game.zone.theme;
    uint8_t level = game.zone.base_level + 5;
    uint8_t mid_move, strong_move;

    if (level > MAX_LEVEL) level = MAX_LEVEL;
    creature_create(boss, species, level);

    /* Boss always knows 3-4 moves */
    if (boss->type < ELEM_NORMAL) {
        mid_move = 7 + boss->type;
        strong_move = 13 + boss->type;
    } else {
        mid_move = MOVE_STRIKE;
        strong_move = MOVE_SLAM;
    }
    if (boss->num_moves < MAX_MOVES) boss->moves[boss->num_moves++] = mid_move;
    if (boss->num_moves < MAX_MOVES) boss->moves[boss->num_moves++] = strong_move;
    if (boss->num_moves < MAX_MOVES) boss->moves[boss->num_moves++] = MOVE_STRIKE;

    /* Boss stat boost */
    boss->max_hp = (uint8_t)MIN(255, (uint16_t)boss->max_hp + 15);
    boss->atk += 3;
    boss->def += 3;
    boss->hp = boss->max_hp;
}

/* ── Overworld state ───────────────────────────────────────── */
static void state_overworld(void) {
    uint8_t tile;

    /* Render map */
    if (game.in_gym) {
        world_render_gym();
    } else {
        world_render_map();
    }
    world_show_player();

    /* Position player sprite */
    {
        uint8_t sx, sy;
        if (game.in_gym) {
            sx = game.px * 8 + 8;
            sy = game.py * 8 + 16;
        } else {
            int16_t cam_x_val = (int16_t)game.px * 8 - 80;
            int16_t cam_y_val = (int16_t)game.py * 8 - 72;
            if (cam_x_val < 0) cam_x_val = 0;
            if (cam_y_val < 0) cam_y_val = 0;
            if (cam_x_val > (MAP_W - SCREEN_W) * 8)
                cam_x_val = (MAP_W - SCREEN_W) * 8;
            if (cam_y_val > (MAP_H - SCREEN_H) * 8)
                cam_y_val = (MAP_H - SCREEN_H) * 8;
            sx = (uint8_t)(game.px * 8 - cam_x_val + 8);
            sy = (uint8_t)(game.py * 8 - cam_y_val + 16);
        }
        move_sprite(0, sx,     sy);
        move_sprite(1, sx + 8, sy);
        move_sprite(2, sx,     sy + 8);
        move_sprite(3, sx + 8, sy + 8);
    }

    for (;;) {
        wait_vbl_done();
        tile = world_update();

        if (tile == 0xFE) {
            /* Menu requested */
            game.state = ST_MENU;
            return;
        }

        if (tile == 0xFF) continue; /* no movement */

        /* Check for encounters on tall grass */
        if (tile == TILE_TALL_GRASS) {
            uint8_t rate = zone_encounter_rate(game.zone.zone_num);
            if (rng_chance(rate)) {
                state_battle_encounter(0);
                if (game.state != ST_OVERWORLD) return;
                /* Redraw map after battle */
                if (game.in_gym) {
                    world_render_gym();
                } else {
                    world_render_map();
                }
                gfx_set_overworld_palettes();
                world_show_player();
                {
                    uint8_t sx2, sy2;
                    if (game.in_gym) {
                        sx2 = game.px * 8 + 8;
                        sy2 = game.py * 8 + 16;
                    } else {
                        int16_t cx = (int16_t)game.px * 8 - 80;
                        int16_t cy = (int16_t)game.py * 8 - 72;
                        if (cx < 0) cx = 0;
                        if (cy < 0) cy = 0;
                        if (cx > (MAP_W - SCREEN_W) * 8)
                            cx = (MAP_W - SCREEN_W) * 8;
                        if (cy > (MAP_H - SCREEN_H) * 8)
                            cy = (MAP_H - SCREEN_H) * 8;
                        sx2 = (uint8_t)(game.px * 8 - cx + 8);
                        sy2 = (uint8_t)(game.py * 8 - cy + 16);
                    }
                    move_sprite(0, sx2,     sy2);
                    move_sprite(1, sx2 + 8, sy2);
                    move_sprite(2, sx2,     sy2 + 8);
                    move_sprite(3, sx2 + 8, sy2 + 8);
                }
            }
        }

        /* Door: enter/exit gym */
        if (tile == TILE_DOOR) {
            if (!game.in_gym) {
                game.in_gym = 1;
                game.px = SCREEN_W / 2;
                game.py = SCREEN_H - 2;
                world_render_gym();
                world_show_player();
                {
                    uint8_t sx3 = game.px * 8 + 8;
                    uint8_t sy3 = game.py * 8 + 16;
                    move_sprite(0, sx3,     sy3);
                    move_sprite(1, sx3 + 8, sy3);
                    move_sprite(2, sx3,     sy3 + 8);
                    move_sprite(3, sx3 + 8, sy3 + 8);
                }
                ui_message("ENTERED THE GYM!", "PREPARE FOR BATTLE");
            } else {
                /* Exit gym back to overworld */
                game.in_gym = 0;
                game.px = game.zone.gym_x;
                game.py = game.zone.gym_y + 1;
                world_render_map();
                world_show_player();
            }
        }

        /* Boss marker */
        if (tile == TILE_BOSS_MARK && !game.zone.boss_defeated) {
            ui_message("GYM LEADER", "APPROACHES!");
            state_battle_encounter(1);
            if (game.state != ST_OVERWORLD) return;
            if (game.zone.boss_defeated) {
                /* Advance to next zone */
                ui_clear_screen();
                ui_print(1, 7, "ZONE CLEARED!");
                ui_print(1, 9, "ADVANCING...");
                ui_wait_press();
                enter_zone(game.zone.zone_num + 1);
                game.state = ST_OVERWORLD;
                return;
            }
            if (game.in_gym) {
                world_render_gym();
            } else {
                world_render_map();
            }
            gfx_set_overworld_palettes();
            world_show_player();
        }

        /* Vendor */
        if (tile == TILE_VENDOR) {
            ui_message("WELCOME TO THE", "IMPERIAL ARMORY!");
            game.state = ST_SHOP;
            return;
        }
    }
}

/* ── Battle encounter ──────────────────────────────────────── */
static void state_battle_encounter(uint8_t is_boss) {
    Creature wild;
    uint8_t result;
    uint8_t choice;
    uint16_t xp_reward;
    uint16_t gold_reward;

    world_hide_player();

    if (is_boss) {
        spawn_boss_creature(&wild);
    } else {
        spawn_wild_creature(&wild);
    }

    battle_init(&wild);
    result = battle_run();

    if (result == BATTLE_LOSE) {
        /* Death: reset to zone start */
        SCX_REG = 0;
        SCY_REG = 0;
        ui_clear_screen();
        ui_set_palette_rect(0, 0, 20, 18, PAL_UI);
        ui_print(2, 7, "THE EMPEROR");
        ui_print(2, 8, "PROTECTS...");
        ui_print(2, 10, "RETURNING TO");
        ui_print(2, 11, "ZONE START.");
        ui_wait_press();

        creature_heal(&game.creature);
        game.px = game.zone.entry_x;
        game.py = game.zone.entry_y;
        game.in_gym = 0;
        game.state = ST_OVERWORLD;
        save_game();
        return;
    }

    if (result == BATTLE_RUN) {
        creature_heal(&game.creature);
        gfx_set_overworld_palettes();
        game.state = ST_OVERWORLD;
        return;
    }

    /* BATTLE_WIN */
    if (is_boss) {
        game.zone.boss_defeated = 1;
    }

    /* Calculate rewards */
    xp_reward = (uint16_t)wild.level * 10;
    gold_reward = (uint16_t)wild.level * 5;
    gold_reward = (gold_reward * character_gold_mult(&game.player)) / 100;

    game.player.gold += gold_reward;

    /* Show gold earned */
    SCX_REG = 0;
    SCY_REG = 0;
    ui_draw_box(0, 13, 20, 5);
    ui_print(1, 14, "EARNED $");
    ui_print_num(9, 14, gold_reward);
    ui_wait_press();

    /* Post-battle choice (not for catches - those are handled in battle) */
    if (result == BATTLE_WIN) {
        /* Check if we caught (creature was replaced in battle_run) */
        /* The catch path already returned BATTLE_WIN with creature replaced.
           For eat/feed, we need the choice menu. */
        choice = battle_post_choice(&wild);

        if (choice == CHOICE_EAT) {
            /* Character gets XP */
            uint16_t char_xp = (xp_reward * character_xp_mult(&game.player)) / 100;
            ui_draw_box(0, 13, 20, 5);
            ui_print(1, 14, "DEVOURED THE");
            ui_print(1, 15, "CREATURE!");
            ui_print(1, 16, "CHAR +");
            ui_print_num(7, 16, char_xp);
            ui_print(12, 16, "XP");
            ui_wait_press();

            if (character_award_xp(&game.player, char_xp)) {
                game.state = ST_SKILLTREE; /* char level up */
                /* Will handle in main loop */
                creature_heal(&game.creature);
                save_game();
                return;
            }
        } else if (choice == CHOICE_FEED) {
            /* Creature gets XP */
            uint16_t crea_xp = (xp_reward * character_xp_mult(&game.player)) / 100;
            ui_draw_box(0, 13, 20, 5);
            ui_print(1, 14, "FED ");
            ui_print(5, 14, species_names[game.creature.species]);
            ui_print(1, 15, "CREATURE +");
            ui_print_num(11, 15, crea_xp);
            ui_print(16, 15, "XP");
            ui_wait_press();

            if (creature_award_xp(&game.creature, crea_xp)) {
                /* Creature level up - goes to creature skill tree */
                ui_draw_box(0, 13, 20, 5);
                ui_print(1, 14, species_names[game.creature.species]);
                ui_print(1, 15, "LEVELED UP!");
                ui_print(1, 16, "LV ");
                ui_print_num(4, 16, game.creature.level);
                ui_wait_press();
                state_skilltree_creature();
            }
        } else {
            /* CATCH attempt (from post-battle menu) */
            uint8_t catch_rate = 30 + character_catch_bonus(&game.player);
            if (character_has_skill(&game.player, CSKILL_VETERANS_EYE))
                catch_rate += 10;
            if (catch_rate > 90) catch_rate = 90;

            ui_draw_box(0, 13, 20, 5);
            ui_print(1, 14, "ATTEMPTING TO");
            ui_print(1, 15, "CAPTURE...");
            ui_wait_press();

            if (rng_chance(catch_rate)) {
                ui_draw_box(0, 13, 20, 5);
                ui_print(1, 14, "CAPTURED!");
                ui_print(1, 15, species_names[wild.species]);
                ui_print(1, 16, "JOINS YOUR HUNT!");
                ui_wait_press();
                memcpy(&game.creature, &wild, sizeof(Creature));
                creature_heal(&game.creature);
                game.creatures_caught++;
            } else {
                ui_draw_box(0, 13, 20, 5);
                ui_print(1, 14, "IT ESCAPED!");
                ui_print(1, 15, "THE PREY FLED.");
                ui_wait_press();
            }
        }
    }

    creature_heal(&game.creature);
    gfx_set_overworld_palettes();
    game.state = ST_OVERWORLD;
    save_game();
}

/* ── Skill tree selection (character) ──────────────────────── */
static void state_skilltree_char(void) {
    SkillOption opts[3];
    uint8_t sel;
    const char *desc_strs[3];

    SCX_REG = 0;
    SCY_REG = 0;
    ui_clear_screen();
    ui_set_palette_rect(0, 0, 20, 18, PAL_UI);

    ui_print(1, 0, "MARINE LEVEL UP!");
    ui_print(1, 1, "LV ");
    ui_print_num(4, 1, game.player.level);
    ui_print(1, 3, "CHOOSE AN ABILITY:");

    skilltree_gen_char_options(opts);

    desc_strs[0] = skilltree_char_desc(opts[0].id);
    desc_strs[1] = skilltree_char_desc(opts[1].id);
    desc_strs[2] = skilltree_char_desc(opts[2].id);

    ui_print(3, 6,  desc_strs[0]);
    ui_print(3, 8,  desc_strs[1]);
    ui_print(3, 10, desc_strs[2]);

    sel = 0;
    for (;;) {
        set_bkg_tile_xy(1, 6,  (sel == 0) ? TILE_CURSOR : TILE_FONT_BASE);
        set_bkg_tile_xy(1, 8,  (sel == 1) ? TILE_CURSOR : TILE_FONT_BASE);
        set_bkg_tile_xy(1, 10, (sel == 2) ? TILE_CURSOR : TILE_FONT_BASE);

        wait_vbl_done();
        {
            uint8_t pressed = ui_poll_keys();
            if (pressed & J_UP) { if (sel > 0) sel--; }
            if (pressed & J_DOWN) { if (sel < 2) sel++; }
            if (pressed & J_A) {
                skilltree_apply_char_skill(&game.player, &opts[sel]);
                ui_message("SKILL ACQUIRED!", desc_strs[sel]);
                game.state = ST_OVERWORLD;
                return;
            }
        }
    }
}

/* ── Skill tree selection (creature) ───────────────────────── */
static void state_skilltree_creature(void) {
    SkillOption opts[3];
    uint8_t sel;
    const char *desc_strs[3];

    SCX_REG = 0;
    SCY_REG = 0;
    ui_clear_screen();
    ui_set_palette_rect(0, 0, 20, 18, PAL_UI);

    ui_print(1, 0, species_names[game.creature.species]);
    ui_print(12, 0, "LEVEL UP!");
    ui_print(1, 1, "LV ");
    ui_print_num(4, 1, game.creature.level);
    ui_print(1, 3, "CHOOSE AN UPGRADE:");

    skilltree_gen_creature_options(opts, &game.creature);

    desc_strs[0] = skilltree_creature_desc(&opts[0]);
    desc_strs[1] = skilltree_creature_desc(&opts[1]);
    desc_strs[2] = skilltree_creature_desc(&opts[2]);

    ui_print(3, 6,  desc_strs[0]);
    if (opts[0].is_move) ui_print(15, 6, "(MOVE)");
    ui_print(3, 8,  desc_strs[1]);
    if (opts[1].is_move) ui_print(15, 8, "(MOVE)");
    ui_print(3, 10, desc_strs[2]);
    if (opts[2].is_move) ui_print(15, 10, "(MOVE)");

    sel = 0;
    for (;;) {
        set_bkg_tile_xy(1, 6,  (sel == 0) ? TILE_CURSOR : TILE_FONT_BASE);
        set_bkg_tile_xy(1, 8,  (sel == 1) ? TILE_CURSOR : TILE_FONT_BASE);
        set_bkg_tile_xy(1, 10, (sel == 2) ? TILE_CURSOR : TILE_FONT_BASE);

        wait_vbl_done();
        {
            uint8_t pressed = ui_poll_keys();
            if (pressed & J_UP) { if (sel > 0) sel--; }
            if (pressed & J_DOWN) { if (sel < 2) sel++; }
            if (pressed & J_A) {
                skilltree_apply_creature_skill(&game.creature, &opts[sel]);
                ui_message("UPGRADE!", desc_strs[sel]);
                return;
            }
        }
    }
}

/* ── Pause menu ────────────────────────────────────────────── */
static void state_menu(void) {
    static const char *const menu_opts[] = {
        "STATUS", "SAVE", "BACK"
    };
    uint8_t sel;

    SCX_REG = 0;
    SCY_REG = 0;
    ui_clear_screen();
    ui_set_palette_rect(0, 0, 20, 18, PAL_UI);
    ui_draw_box(0, 0, 20, 18);

    ui_print(5, 1, "IMPERIAL HUNT");

    /* Marine stats */
    ui_print(1, 3, "MARINE LV ");
    ui_print_num(11, 3, game.player.level);
    ui_print(1, 4, "CHA:");
    ui_print_num(5, 4, character_get_cha(&game.player));
    ui_print(9, 4, "WIS:");
    ui_print_num(13, 4, character_get_wis(&game.player));
    ui_print(1, 5, "LCK:");
    ui_print_num(5, 5, character_get_lck(&game.player));
    ui_print(9, 5, "CON:");
    ui_print_num(13, 5, character_get_con(&game.player));
    ui_print(1, 6, "GOLD: $");
    ui_print_num(8, 6, game.player.gold);

    /* Creature stats */
    ui_print(1, 8, species_names[game.creature.species]);
    ui_print(12, 8, "LV ");
    ui_print_num(15, 8, game.creature.level);
    ui_print(1, 9, "HP:");
    ui_print_num(4, 9, game.creature.max_hp);
    ui_print(9, 9, "SP:");
    ui_print_num(12, 9, game.creature.max_sp);
    ui_print(1, 10, "ATK:");
    ui_print_num(5, 10, game.creature.atk);
    ui_print(9, 10, "DEF:");
    ui_print_num(13, 10, game.creature.def);
    ui_print(1, 11, "SPD:");
    ui_print_num(5, 11, game.creature.spd);

    /* Zone info */
    ui_print(1, 13, "ZONE ");
    ui_print_num(6, 13, game.zone.zone_num + 1);
    ui_print(9, 13, zone_theme_name(game.zone.theme));

    sel = ui_menu(1, 15, menu_opts, 3);

    if (sel == 1) {
        save_game();
        ui_message("GAME SAVED!", "");
    }

    game.state = ST_OVERWORLD;
}

/* ── Game over ─────────────────────────────────────────────── */
static void state_gameover(void) {
    SCX_REG = 0;
    SCY_REG = 0;
    ui_clear_screen();
    ui_set_palette_rect(0, 0, 20, 18, PAL_UI);
    ui_print(4, 7, "GAME OVER");
    ui_print(2, 9, "THE HUNT CONTINUES");
    ui_print(4, 11, "PRESS START");
    ui_wait_press();
    game.state = ST_TITLE;
}

/* ══════════════════════════════════════════════════════════════
   MAIN ENTRY POINT
   ══════════════════════════════════════════════════════════════ */
void main(void) {
    init_hardware();
    game.state = ST_TITLE;

    for (;;) {
        switch (game.state) {
            case ST_TITLE:
                state_title();
                break;
            case ST_STARTER:
                state_starter();
                break;
            case ST_OVERWORLD:
                state_overworld();
                break;
            case ST_SHOP:
                world_hide_player();
                state_shop();
                break;
            case ST_SKILLTREE:
                world_hide_player();
                state_skilltree_char();
                break;
            case ST_GAMEOVER:
                state_gameover();
                break;
            case ST_MENU:
                world_hide_player();
                state_menu();
                break;
            default:
                game.state = ST_TITLE;
                break;
        }
    }
}

/* Shop state handler */
static void state_shop(void) {
    shop_run(game.zone.zone_num);
    gfx_set_overworld_palettes();
    game.state = ST_OVERWORLD;
}
