/* battle.c - Turn-based battle system */
#include "battle.h"
#include "creature.h"
#include "character.h"
#include "ui.h"
#include "gfx_data.h"
#include "rng.h"

/* ── Local battle state ────────────────────────────────────── */
static Creature enemy;
static uint8_t battle_cry_turns;  /* remaining turns for Battle Cry */

/* ── Draw creature sprites on BG ───────────────────────────── */
static void draw_creature_sprite(uint8_t bx, uint8_t by,
                                 uint8_t base_tile, uint8_t pal) {
    uint8_t x, y;
    uint8_t tiles[4];
    uint8_t attrs[4];

    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            tiles[x] = base_tile + y * 4 + x;
            attrs[x] = pal;
        }
        set_bkg_tiles(bx, by + y, 4, 1, tiles);
        VBK_REG = 1;
        set_bkg_tiles(bx, by + y, 4, 1, attrs);
        VBK_REG = 0;
    }
}

/* ── Draw full battle scene ────────────────────────────────── */
void battle_draw_scene(const Creature *player_c, const Creature *enemy_c) {
    ui_clear_screen();

    /* Enemy info (top) */
    ui_print(0, 0, species_names[enemy_c->species]);
    ui_print(14, 0, "LV");
    ui_print_num(16, 0, enemy_c->level);
    ui_draw_hp_bar(0, 1, enemy_c->hp, enemy_c->max_hp, 10);
    ui_print(11, 1, "HP");

    /* Enemy sprite (top-left) */
    gfx_load_creature_sprite(TILE_CREATURE2, enemy_c->species);
    draw_creature_sprite(1, 2, TILE_CREATURE2, 2);

    /* Player creature info */
    ui_print(0, 11, species_names[player_c->species]);
    ui_print(14, 11, "LV");
    ui_print_num(16, 11, player_c->level);
    ui_draw_hp_bar(0, 12, player_c->hp, player_c->max_hp, 8);
    ui_print(9, 12, "HP");
    ui_draw_sp_bar(12, 12, player_c->sp, player_c->max_sp, 6);
    ui_print(19, 12, "S");

    /* Player creature sprite (bottom-right) */
    gfx_load_creature_sprite(TILE_CREATURE1, player_c->species);
    draw_creature_sprite(14, 7, TILE_CREATURE1, 1);
}

/* ── Calculate damage ──────────────────────────────────────── */
static uint8_t calc_damage(const Creature *attacker, const Creature *defender,
                           uint8_t move_id) {
    const MoveData *m = &move_table[move_id];
    uint16_t dmg;
    uint8_t eff, stab;
    uint8_t atk_val = attacker->atk;
    uint8_t def_val = defender->def;
    uint8_t i;

    /* Base damage: (ATK * Power) / DEF */
    dmg = ((uint16_t)atk_val * (uint16_t)m->power);
    if (def_val < 1) def_val = 1;
    dmg = dmg / (uint16_t)def_val;

    /* Type effectiveness */
    eff = type_effectiveness(m->type, defender->type);
    dmg = (dmg * (uint16_t)eff) / 100;

    /* STAB */
    stab = type_stab(attacker->type, m->type);
    dmg = (dmg * (uint16_t)stab) / 100;

    /* Critical hit: SPD/4 % chance, 1.5x */
    if (rng_range(0, 99) < (attacker->spd / 4)) {
        dmg = (dmg * 150) / 100;
    }

    /* Random variance +/- 15% */
    dmg = (dmg * (uint16_t)rng_range(85, 100)) / 100;

    /* Fury: +25% ATK when below 50% HP */
    for (i = 0; i < attacker->num_skills; i++) {
        if (attacker->skills[i] == MSKILL_FURY &&
            attacker->hp <= attacker->max_hp / 2) {
            dmg = (dmg * 125) / 100;
            break;
        }
    }

    /* Type Mastery: extra STAB */
    for (i = 0; i < attacker->num_skills; i++) {
        if (attacker->skills[i] == MSKILL_TYPE_MASTERY &&
            attacker->type == m->type && m->type != ELEM_NORMAL) {
            dmg = (dmg * 125) / 100;
            break;
        }
    }

    /* Thick Hide: defender -2 flat */
    for (i = 0; i < defender->num_skills; i++) {
        if (defender->skills[i] == MSKILL_THICK_HIDE) {
            if (dmg > 2) dmg -= 2; else dmg = 1;
            break;
        }
    }

    /* Iron Halo: -10% if player creature is defender with char skill */
    if (defender == &game.creature &&
        character_has_skill(&game.player, CSKILL_IRON_HALO)) {
        dmg = (dmg * 90) / 100;
    }

    /* Blessed Rounds: ignore 10% DEF if player creature attacking */
    if (attacker == &game.creature &&
        character_has_skill(&game.player, CSKILL_BLESSED)) {
        dmg = (dmg * 110) / 100;
    }

    /* Battle Cry: +15% for first 3 turns */
    if (attacker == &game.creature && battle_cry_turns > 0) {
        dmg = (dmg * 115) / 100;
    }

    /* Minimum 1 damage */
    if (dmg < 1) dmg = 1;
    if (dmg > 255) dmg = 255;

    return (uint8_t)dmg;
}

/* ── Enemy AI: pick a random move ──────────────────────────── */
static uint8_t enemy_pick_move(void) {
    uint8_t i, best = 0;
    uint8_t best_dmg = 0;

    /* Try each move, pick highest damage with some randomness */
    for (i = 0; i < enemy.num_moves; i++) {
        if (move_table[enemy.moves[i]].sp_cost <= enemy.sp) {
            uint8_t est = move_table[enemy.moves[i]].power;
            uint8_t eff = type_effectiveness(move_table[enemy.moves[i]].type,
                                             game.creature.type);
            est = (uint8_t)(((uint16_t)est * (uint16_t)eff) / 100);
            if (est > best_dmg || (est == best_dmg && rng_chance(40))) {
                best_dmg = est;
                best = i;
            }
        }
    }
    return best;
}

/* ── Apply regen skills ────────────────────────────────────── */
static void apply_regen(Creature *c) {
    uint8_t i;
    for (i = 0; i < c->num_skills; i++) {
        if (c->skills[i] == MSKILL_REGEN) {
            uint8_t heal = c->max_hp / 20;
            if (heal < 1) heal = 1;
            c->hp += heal;
            if (c->hp > c->max_hp) c->hp = c->max_hp;
            break;
        }
    }
}

/* ── Check evasion ─────────────────────────────────────────── */
static uint8_t check_evasion(const Creature *defender) {
    uint8_t i;
    for (i = 0; i < defender->num_skills; i++) {
        if (defender->skills[i] == MSKILL_EVASION) {
            return rng_chance(10);
        }
    }
    return 0;
}

/* ── Apply counter damage ──────────────────────────────────── */
static uint8_t check_counter(const Creature *defender, uint8_t dmg) {
    uint8_t i;
    for (i = 0; i < defender->num_skills; i++) {
        if (defender->skills[i] == MSKILL_COUNTER) {
            return (dmg * 20) / 100;
        }
    }
    return 0;
}

/* ── Battle menu ───────────────────────────────────────────── */
static const char *const battle_menu_opts[] = { "FIGHT", "CATCH", "RUN" };
static const char *const post_battle_opts[] = { "EAT", "FEED", "CATCH" };

/* ── Run battle ────────────────────────────────────────────── */
uint8_t battle_run(void) {
    uint8_t player_spd, enemy_spd;
    uint8_t player_move, enemy_move;
    uint8_t dmg, counter_dmg;
    uint8_t choice;
    uint8_t player_first;
    const MoveData *pm;

    battle_cry_turns = 0;
    if (character_has_skill(&game.player, CSKILL_BATTLE_CRY)) {
        battle_cry_turns = 3;
    }

    /* Set battle palettes */
    gfx_set_battle_palettes(game.creature.type, enemy.type);

    /* Draw initial scene */
    battle_draw_scene(&game.creature, &enemy);

    /* Intro message */
    ui_draw_box(0, 13, 20, 5);
    ui_print(1, 14, "A WILD ");
    ui_print(8, 14, species_names[enemy.species]);
    ui_print(1, 15, "APPEARED!");
    ui_wait_press();

    /* ── Main battle loop ──────────────────────────────────── */
    while (game.creature.hp > 0 && enemy.hp > 0) {
        /* Redraw scene */
        battle_draw_scene(&game.creature, &enemy);

        /* Show menu */
        ui_draw_box(0, 13, 20, 5);
        ui_print(1, 14, "WHAT WILL");
        ui_print(1, 15, species_names[game.creature.species]);
        ui_print(1, 16, "DO?");
        ui_draw_box(10, 14, 10, 4);
        choice = ui_menu(11, 15, battle_menu_opts, 3);

        if (choice == 0xFF) continue; /* cancelled, re-show */

        if (choice == 2) {
            /* RUN */
            if (rng_chance(50 + game.creature.spd - enemy.spd)) {
                ui_draw_box(0, 13, 20, 5);
                ui_print(1, 14, "GOT AWAY");
                ui_print(1, 15, "SAFELY!");
                ui_wait_press();
                return BATTLE_RUN;
            } else {
                ui_draw_box(0, 13, 20, 5);
                ui_print(1, 14, "CAN'T ESCAPE!");
                ui_wait_press();
                /* Enemy gets a free turn */
                enemy_move = enemy_pick_move();
                pm = &move_table[enemy.moves[enemy_move]];
                if (rng_range(0, 99) < pm->accuracy &&
                    !check_evasion(&game.creature)) {
                    dmg = calc_damage(&enemy, &game.creature,
                                      enemy.moves[enemy_move]);
                    if (dmg >= game.creature.hp) game.creature.hp = 0;
                    else game.creature.hp -= dmg;
                    enemy.sp -= pm->sp_cost;
                }
                battle_draw_scene(&game.creature, &enemy);
                continue;
            }
        }

        if (choice == 1) {
            /* CATCH attempt */
            uint8_t catch_rate = 30 + character_catch_bonus(&game.player);
            /* Lower HP = higher catch rate */
            catch_rate += (uint8_t)((uint16_t)(enemy.max_hp - enemy.hp) * 20 /
                                    (uint16_t)enemy.max_hp);
            if (character_has_skill(&game.player, CSKILL_VETERANS_EYE))
                catch_rate += 10;
            if (catch_rate > 95) catch_rate = 95;

            ui_draw_box(0, 13, 20, 5);
            ui_print(1, 14, "THREW A TRAP...");
            ui_wait_press();

            if (rng_chance(catch_rate)) {
                ui_draw_box(0, 13, 20, 5);
                ui_print(1, 14, "CAUGHT");
                ui_print(1, 15, species_names[enemy.species]);
                ui_print(1, 16, "!");
                ui_wait_press();
                /* Replace player creature */
                memcpy(&game.creature, &enemy, sizeof(Creature));
                creature_heal(&game.creature);
                game.creatures_caught++;
                return BATTLE_WIN;
            } else {
                ui_draw_box(0, 13, 20, 5);
                ui_print(1, 14, "IT BROKE FREE!");
                ui_wait_press();
                /* Enemy gets a free turn */
                enemy_move = enemy_pick_move();
                pm = &move_table[enemy.moves[enemy_move]];
                if (rng_range(0, 99) < pm->accuracy &&
                    !check_evasion(&game.creature)) {
                    dmg = calc_damage(&enemy, &game.creature,
                                      enemy.moves[enemy_move]);
                    if (dmg >= game.creature.hp) game.creature.hp = 0;
                    else game.creature.hp -= dmg;
                    enemy.sp -= pm->sp_cost;
                }
                battle_draw_scene(&game.creature, &enemy);
                continue;
            }
        }

        /* FIGHT: choose a move */
        {
            const char *move_opts[MAX_MOVES];
            uint8_t i;
            for (i = 0; i < game.creature.num_moves; i++) {
                move_opts[i] = move_names[game.creature.moves[i]];
            }
            ui_draw_box(0, 13, 20, 5);
            choice = ui_menu(1, 14, move_opts, game.creature.num_moves);
            if (choice == 0xFF) continue; /* back to main menu */

            player_move = choice;
        }

        /* Check SP */
        pm = &move_table[game.creature.moves[player_move]];
        if (pm->sp_cost > game.creature.sp) {
            ui_draw_box(0, 13, 20, 5);
            ui_print(1, 14, "NOT ENOUGH SP!");
            ui_wait_press();
            continue;
        }

        /* Determine turn order */
        player_spd = game.creature.spd;
        enemy_spd = enemy.spd;
        if (player_spd > enemy_spd) {
            player_first = 1;
        } else if (enemy_spd > player_spd) {
            player_first = 0;
        } else {
            /* Speed tie */
            if (character_has_skill(&game.player, CSKILL_TACTICAL)) {
                player_first = 1;
            } else {
                player_first = rng_chance(50);
            }
        }

        enemy_move = enemy_pick_move();

        /* ── Execute turns ─────────────────────────────────── */
        if (player_first) {
            /* Player attacks */
            game.creature.sp -= pm->sp_cost;
            ui_draw_box(0, 13, 20, 5);
            ui_print(1, 14, species_names[game.creature.species]);
            ui_print(1, 15, "USED ");
            ui_print(6, 15, move_names[game.creature.moves[player_move]]);
            ui_wait_press();

            if (rng_range(0, 99) < pm->accuracy &&
                !check_evasion(&enemy)) {
                dmg = calc_damage(&game.creature, &enemy,
                                  game.creature.moves[player_move]);
                if (dmg >= enemy.hp) enemy.hp = 0;
                else enemy.hp -= dmg;

                counter_dmg = check_counter(&enemy, dmg);
                if (counter_dmg > 0) {
                    if (counter_dmg >= game.creature.hp)
                        game.creature.hp = 0;
                    else
                        game.creature.hp -= counter_dmg;
                }
            } else {
                ui_draw_box(0, 13, 20, 5);
                ui_print(1, 14, "IT MISSED!");
                ui_wait_press();
            }

            battle_draw_scene(&game.creature, &enemy);

            /* Check enemy KO */
            if (enemy.hp == 0) break;

            /* Enemy attacks */
            {
                const MoveData *em = &move_table[enemy.moves[enemy_move]];
                if (em->sp_cost <= enemy.sp) {
                    enemy.sp -= em->sp_cost;
                    ui_draw_box(0, 13, 20, 5);
                    ui_print(1, 14, species_names[enemy.species]);
                    ui_print(1, 15, "USED ");
                    ui_print(6, 15, move_names[enemy.moves[enemy_move]]);
                    ui_wait_press();

                    if (rng_range(0, 99) < em->accuracy &&
                        !check_evasion(&game.creature)) {
                        dmg = calc_damage(&enemy, &game.creature,
                                          enemy.moves[enemy_move]);
                        if (dmg >= game.creature.hp) game.creature.hp = 0;
                        else game.creature.hp -= dmg;

                        counter_dmg = check_counter(&game.creature, dmg);
                        if (counter_dmg > 0) {
                            if (counter_dmg >= enemy.hp) enemy.hp = 0;
                            else enemy.hp -= counter_dmg;
                        }
                    } else {
                        ui_draw_box(0, 13, 20, 5);
                        ui_print(1, 14, "IT MISSED!");
                        ui_wait_press();
                    }
                }
            }
        } else {
            /* Enemy attacks first */
            {
                const MoveData *em = &move_table[enemy.moves[enemy_move]];
                if (em->sp_cost <= enemy.sp) {
                    enemy.sp -= em->sp_cost;
                    ui_draw_box(0, 13, 20, 5);
                    ui_print(1, 14, species_names[enemy.species]);
                    ui_print(1, 15, "USED ");
                    ui_print(6, 15, move_names[enemy.moves[enemy_move]]);
                    ui_wait_press();

                    if (rng_range(0, 99) < em->accuracy &&
                        !check_evasion(&game.creature)) {
                        dmg = calc_damage(&enemy, &game.creature,
                                          enemy.moves[enemy_move]);
                        if (dmg >= game.creature.hp) game.creature.hp = 0;
                        else game.creature.hp -= dmg;

                        counter_dmg = check_counter(&game.creature, dmg);
                        if (counter_dmg > 0) {
                            if (counter_dmg >= enemy.hp) enemy.hp = 0;
                            else enemy.hp -= counter_dmg;
                        }
                    } else {
                        ui_draw_box(0, 13, 20, 5);
                        ui_print(1, 14, "IT MISSED!");
                        ui_wait_press();
                    }
                }
            }

            battle_draw_scene(&game.creature, &enemy);

            /* Check player KO */
            if (game.creature.hp == 0) break;

            /* Player attacks */
            game.creature.sp -= pm->sp_cost;
            ui_draw_box(0, 13, 20, 5);
            ui_print(1, 14, species_names[game.creature.species]);
            ui_print(1, 15, "USED ");
            ui_print(6, 15, move_names[game.creature.moves[player_move]]);
            ui_wait_press();

            if (rng_range(0, 99) < pm->accuracy &&
                !check_evasion(&enemy)) {
                dmg = calc_damage(&game.creature, &enemy,
                                  game.creature.moves[player_move]);
                if (dmg >= enemy.hp) enemy.hp = 0;
                else enemy.hp -= dmg;

                counter_dmg = check_counter(&enemy, dmg);
                if (counter_dmg > 0) {
                    if (counter_dmg >= game.creature.hp)
                        game.creature.hp = 0;
                    else
                        game.creature.hp -= counter_dmg;
                }
            } else {
                ui_draw_box(0, 13, 20, 5);
                ui_print(1, 14, "IT MISSED!");
                ui_wait_press();
            }
        }

        battle_draw_scene(&game.creature, &enemy);

        /* Apply regen at end of turn */
        apply_regen(&game.creature);
        apply_regen(&enemy);

        /* Decrement battle cry */
        if (battle_cry_turns > 0) battle_cry_turns--;
    }

    /* ── Battle end ────────────────────────────────────────── */
    if (enemy.hp == 0) {
        /* Victory */
        ui_draw_box(0, 13, 20, 5);
        ui_print(1, 14, species_names[enemy.species]);
        ui_print(1, 15, "WAS DEFEATED!");
        ui_wait_press();
        game.battles_won++;
        return BATTLE_WIN;
    } else {
        /* Defeat */
        ui_draw_box(0, 13, 20, 5);
        ui_print(1, 14, species_names[game.creature.species]);
        ui_print(1, 15, "FAINTED...");
        ui_wait_press();
        return BATTLE_LOSE;
    }
}

/* ── Initialize battle with enemy ──────────────────────────── */
void battle_init(Creature *e) {
    memcpy(&enemy, e, sizeof(Creature));
}

/* ── Post-battle choice ────────────────────────────────────── */
uint8_t battle_post_choice(const Creature *defeated) {
    uint8_t choice;
    (void)defeated;

    ui_draw_box(0, 13, 20, 5);
    ui_print(1, 14, "WHAT DO YOU DO");
    ui_print(1, 15, "WITH THE PREY?");
    ui_draw_box(10, 14, 10, 4);
    choice = ui_menu(11, 15, post_battle_opts, 3);

    if (choice == 0xFF) choice = 0; /* default to EAT if cancelled */
    return choice;
}
