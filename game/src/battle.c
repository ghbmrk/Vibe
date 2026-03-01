#include "battle.h"
#include "creature.h"
#include "ui.h"
#include "rng.h"
#include "gfx_data.h"
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdio.h>

/* Battle-local state */
static BattleSide player_side;
static BattleSide enemy_side;
static uint8_t battle_over;
static uint8_t battle_won;
static uint8_t is_boss_battle;

/* Forward declarations */
static void battle_draw_scene(void);
static void battle_draw_hud(void);
static void battle_player_turn(void);
static void battle_enemy_turn(void);
static void battle_execute_move(BattleSide *atk, BattleSide *dfn, uint8_t move_idx);
static void battle_apply_status(BattleSide *side);
static uint8_t battle_check_faint(BattleSide *side);
static void battle_award_exp(Creature *winner, Creature *loser);

/* =========================================================
   BATTLE SCENE DRAWING
   ========================================================= */
static void battle_draw_scene(void) {
    uint8_t pal;

    ui_clear_screen();

    /* Enemy creature info - top left */
    ui_print(1, 0, species_db[enemy_side.mon->species].name);
    ui_print(1, 1, "Lv");
    ui_print_num(3, 1, enemy_side.mon->level, 2);
    ui_draw_hp_bar(1, 2, enemy_side.mon->hp, enemy_side.mon->max_hp);

    /* Enemy sprite placeholder - show as colored box area */
    pal = creature_get_palette(enemy_side.mon->species);
    ui_set_area_palette(13, 1, 4, 4, pal);

    /* Player creature info - bottom area */
    ui_print(1, 8, species_db[player_side.mon->species].name);
    ui_print(1, 9, "Lv");
    ui_print_num(3, 9, player_side.mon->level, 2);
    ui_draw_hp_bar(1, 10, player_side.mon->hp, player_side.mon->max_hp);
    ui_print_num(1, 11, player_side.mon->hp, 3);
    ui_print(4, 11, "/");
    ui_print_num(5, 11, player_side.mon->max_hp, 3);

    /* Player sprite area */
    pal = creature_get_palette(player_side.mon->species);
    ui_set_area_palette(2, 5, 4, 4, pal);

    /* Display creature sprites using OBJ sprites */
    {
        uint8_t base_tile;
        uint8_t spal;

        /* Enemy creature sprite (top-right area) */
        base_tile = PLAYER_TILE_COUNT + (enemy_side.mon->species * 4);
        spal = creature_get_palette(enemy_side.mon->species);
        set_sprite_tile(SPR_CREATURE, base_tile);
        set_sprite_tile(SPR_CREATURE + 1, base_tile + 1);
        set_sprite_tile(SPR_CREATURE + 2, base_tile + 2);
        set_sprite_tile(SPR_CREATURE + 3, base_tile + 3);
        move_sprite(SPR_CREATURE,     112, 16);
        move_sprite(SPR_CREATURE + 1, 120, 16);
        move_sprite(SPR_CREATURE + 2, 112, 24);
        move_sprite(SPR_CREATURE + 3, 120, 24);
        set_sprite_prop(SPR_CREATURE,     spal);
        set_sprite_prop(SPR_CREATURE + 1, spal);
        set_sprite_prop(SPR_CREATURE + 2, spal);
        set_sprite_prop(SPR_CREATURE + 3, spal);
    }
}

static void battle_draw_hud(void) {
    /* Update HP bars */
    ui_draw_hp_bar(1, 2, enemy_side.mon->hp, enemy_side.mon->max_hp);
    ui_draw_hp_bar(1, 10, player_side.mon->hp, player_side.mon->max_hp);
    ui_print_num(1, 11, player_side.mon->hp, 3);
    ui_print(4, 11, "/");
    ui_print_num(5, 11, player_side.mon->max_hp, 3);
}

/* =========================================================
   MOVE EXECUTION
   ========================================================= */
static void battle_execute_move(BattleSide *atk, BattleSide *dfn, uint8_t move_idx) {
    const MoveData *mv;
    uint16_t damage;
    uint8_t hit_roll;
    const char *atk_name;
    const char *dfn_name;
    uint8_t eff1, eff2;

    if (move_idx == 0xFF) return;

    mv = &move_db[move_idx];
    atk_name = species_db[atk->mon->species].name;
    dfn_name = species_db[dfn->mon->species].name;

    /* Show move usage */
    ui_show_message(atk_name, mv->name);
    ui_wait_button();

    /* Accuracy check */
    hit_roll = rng_range(1, 100);
    if (hit_roll > mv->accuracy) {
        ui_show_message(atk_name, "missed!");
        ui_wait_button();
        return;
    }

    /* Calculate and apply damage */
    damage = calc_damage(atk, dfn, move_idx);

    if (damage > 0) {
        if (dfn->mon->hp > damage) {
            dfn->mon->hp -= damage;
        } else {
            dfn->mon->hp = 0;
        }

        /* Type effectiveness messages */
        eff1 = type_chart[mv->type][species_db[dfn->mon->species].type1];
        eff2 = type_chart[mv->type][species_db[dfn->mon->species].type2];

        if (eff1 == 0 || eff2 == 0) {
            ui_show_message("No effect...", "");
        } else if (eff1 >= 20 || eff2 >= 20) {
            ui_show_message("Super effective!", "");
            ui_wait_button();
        } else if (eff1 <= 5 || eff2 <= 5) {
            ui_show_message("Not effective...", "");
            ui_wait_button();
        }
    }

    /* Status effect */
    if (mv->effect != STATUS_NONE && dfn->mon->status == STATUS_NONE) {
        if (rng_range(1, 100) <= mv->effect_chance) {
            dfn->mon->status = mv->effect;
            dfn->mon->status_turns = rng_range(2, 4);
            switch (mv->effect) {
                case STATUS_BURN:
                    ui_show_message(dfn_name, "was burned!");
                    break;
                case STATUS_POISON:
                    ui_show_message(dfn_name, "was poisoned!");
                    break;
                case STATUS_PARALYZE:
                    ui_show_message(dfn_name, "is paralyzed!");
                    break;
                case STATUS_SLEEP:
                    ui_show_message(dfn_name, "fell asleep!");
                    break;
            }
            ui_wait_button();
        }
    }

    battle_draw_hud();
}

/* Apply status damage/checks at end of turn */
static void battle_apply_status(BattleSide *side) {
    if (side->mon->status == STATUS_NONE || side->mon->hp == 0) return;

    switch (side->mon->status) {
        case STATUS_BURN:
        case STATUS_POISON: {
            uint16_t dmg = side->mon->max_hp / 8;
            if (dmg == 0) dmg = 1;
            if (side->mon->hp > dmg) {
                side->mon->hp -= dmg;
            } else {
                side->mon->hp = 0;
            }
            if (side->mon->status == STATUS_BURN) {
                ui_show_message(species_db[side->mon->species].name, "is hurt by burn!");
            } else {
                ui_show_message(species_db[side->mon->species].name, "is hurt by poison!");
            }
            ui_wait_button();
            battle_draw_hud();
            break;
        }
    }

    /* Decrement status turns */
    if (side->mon->status_turns > 0) {
        side->mon->status_turns--;
        if (side->mon->status_turns == 0) {
            ui_show_message(species_db[side->mon->species].name, "recovered!");
            side->mon->status = STATUS_NONE;
            ui_wait_button();
        }
    }
}

static uint8_t battle_check_faint(BattleSide *side) {
    if (side->mon->hp == 0) {
        ui_show_message(species_db[side->mon->species].name, "fainted!");
        ui_wait_button();
        return 1;
    }
    return 0;
}

/* =========================================================
   PLAYER TURN
   ========================================================= */
static void battle_player_turn(void) {
    const char *main_items[] = {"Fight", "Item", "Switch", "Run"};
    uint8_t choice;
    uint8_t i;

    /* Check paralysis/sleep */
    if (player_side.mon->status == STATUS_PARALYZE) {
        if (rng_range(1, 4) == 1) {
            ui_show_message(species_db[player_side.mon->species].name, "is paralyzed!");
            ui_wait_button();
            return;
        }
    }
    if (player_side.mon->status == STATUS_SLEEP) {
        ui_show_message(species_db[player_side.mon->species].name, "is asleep...");
        ui_wait_button();
        return;
    }

    ui_draw_box(0, 12, 20, 6);
    choice = ui_menu(1, 13, main_items, 4);

    switch (choice) {
        case 0: { /* Fight */
            const char *move_names[MAX_MOVES];
            uint8_t move_count = 0;

            for (i = 0; i < MAX_MOVES; i++) {
                if (player_side.mon->moves[i] != 0xFF) {
                    move_names[move_count] = move_db[player_side.mon->moves[i]].name;
                    move_count++;
                }
            }

            if (move_count == 0) {
                ui_show_message("No moves!", "");
                ui_wait_button();
                return;
            }

            ui_draw_box(0, 12, 20, 6);
            choice = ui_menu(1, 13, move_names, move_count);
            if (choice == 0xFF) {
                battle_player_turn(); /* Back to main menu */
                return;
            }

            /* Check PP */
            if (player_side.mon->pp[choice] == 0) {
                ui_show_message("No PP left!", "");
                ui_wait_button();
                battle_player_turn();
                return;
            }

            player_side.mon->pp[choice]--;
            battle_execute_move(&player_side, &enemy_side, player_side.mon->moves[choice]);
            break;
        }
        case 1: { /* Item */
            const char *item_names[] = {"Potion", "Elixir", "Revive", "Antidote", "SmokeBall"};
            uint8_t item_choice;

            ui_draw_box(0, 12, 20, 6);
            /* Show items with counts */
            for (i = 0; i < 5; i++) {
                if (save.items[i + 1] > 0) {
                    ui_print(2, 13 + i, item_names[i]);
                    ui_print_num(14, 13 + i, save.items[i + 1], 2);
                }
            }

            item_choice = ui_menu(1, 13, item_names, 5);
            if (item_choice == 0xFF) {
                battle_player_turn();
                return;
            }
            if (save.items[item_choice + 1] == 0) {
                ui_show_message("None left!", "");
                ui_wait_button();
                battle_player_turn();
                return;
            }

            save.items[item_choice + 1]--;

            switch (item_choice + 1) {
                case ITEM_POTION: {
                    uint16_t heal = player_side.mon->max_hp / 3;
                    if (heal < 20) heal = 20;
                    player_side.mon->hp += heal;
                    if (player_side.mon->hp > player_side.mon->max_hp)
                        player_side.mon->hp = player_side.mon->max_hp;
                    ui_show_message("HP restored!", "");
                    break;
                }
                case ITEM_ELIXIR: {
                    player_side.mon->hp = player_side.mon->max_hp;
                    ui_show_message("Full HP!", "");
                    break;
                }
                case ITEM_ANTIDOTE: {
                    player_side.mon->status = STATUS_NONE;
                    player_side.mon->status_turns = 0;
                    ui_show_message("Status cured!", "");
                    break;
                }
                case ITEM_SMOKEBALL: {
                    if (!is_boss_battle) {
                        ui_show_message("Got away!", "");
                        ui_wait_button();
                        battle_over = 1;
                        return;
                    } else {
                        ui_show_message("Can not flee!", "");
                    }
                    break;
                }
            }
            ui_wait_button();
            battle_draw_hud();
            break;
        }
        case 2: { /* Switch */
            if (save.party_count <= 1) {
                ui_show_message("No others!", "");
                ui_wait_button();
                battle_player_turn();
                return;
            }
            {
                const char *party_names[MAX_PARTY];
                uint8_t pc = 0;
                for (i = 0; i < save.party_count; i++) {
                    if (&save.party[i] != player_side.mon && save.party[i].hp > 0) {
                        party_names[pc] = species_db[save.party[i].species].name;
                        pc++;
                    }
                }
                if (pc == 0) {
                    ui_show_message("All fainted!", "");
                    ui_wait_button();
                    battle_player_turn();
                    return;
                }
                ui_draw_box(0, 12, 20, 6);
                choice = ui_menu(1, 13, party_names, pc);
                if (choice != 0xFF) {
                    /* Find the actual party member */
                    uint8_t found = 0;
                    for (i = 0; i < save.party_count; i++) {
                        if (&save.party[i] != player_side.mon && save.party[i].hp > 0) {
                            if (found == choice) {
                                player_side.mon = &save.party[i];
                                player_side.atk_stage = 0;
                                player_side.def_stage = 0;
                                player_side.spatk_stage = 0;
                                player_side.spdef_stage = 0;
                                player_side.speed_stage = 0;
                                ui_show_message("Go,", species_db[player_side.mon->species].name);
                                ui_wait_button();
                                battle_draw_scene();
                                break;
                            }
                            found++;
                        }
                    }
                } else {
                    battle_player_turn();
                    return;
                }
            }
            break;
        }
        case 3: /* Run */
        default: {
            if (is_boss_battle) {
                ui_show_message("Can not flee", "from a boss!");
                ui_wait_button();
                battle_player_turn();
                return;
            }
            /* Run success based on speed */
            if (rng_range(0, 100) < 50 + player_side.mon->speed) {
                ui_show_message("Got away!", "");
                ui_wait_button();
                battle_over = 1;
                return;
            } else {
                ui_show_message("Can not escape!", "");
                ui_wait_button();
            }
            break;
        }
    }
}

/* =========================================================
   ENEMY TURN (AI)
   ========================================================= */
static void battle_enemy_turn(void) {
    uint8_t i;
    uint8_t best_move = 0;
    uint16_t best_dmg = 0;

    /* Check paralysis/sleep */
    if (enemy_side.mon->status == STATUS_PARALYZE) {
        if (rng_range(1, 4) == 1) {
            ui_show_message(species_db[enemy_side.mon->species].name, "is paralyzed!");
            ui_wait_button();
            return;
        }
    }
    if (enemy_side.mon->status == STATUS_SLEEP) {
        ui_show_message(species_db[enemy_side.mon->species].name, "is asleep...");
        ui_wait_button();
        return;
    }

    /* Simple AI: pick highest damage move */
    for (i = 0; i < MAX_MOVES; i++) {
        if (enemy_side.mon->moves[i] != 0xFF && enemy_side.mon->pp[i] > 0) {
            uint16_t dmg = calc_damage(&enemy_side, &player_side, enemy_side.mon->moves[i]);
            if (dmg > best_dmg) {
                best_dmg = dmg;
                best_move = i;
            }
        }
    }

    if (enemy_side.mon->moves[best_move] != 0xFF && enemy_side.mon->pp[best_move] > 0) {
        enemy_side.mon->pp[best_move]--;
        battle_execute_move(&enemy_side, &player_side, enemy_side.mon->moves[best_move]);
    }
}

/* =========================================================
   EXP AWARD
   ========================================================= */
static void battle_award_exp(Creature *winner, Creature *loser) {
    uint16_t base_exp;
    uint16_t gained;
    uint8_t leveled;

    /* EXP = (base_hp of loser species * loser level) / 5 */
    base_exp = species_db[loser->species].base_hp;
    gained = (base_exp * loser->level) / 5;
    if (is_boss_battle) gained *= 2;
    if (gained < 1) gained = 1;

    winner->exp += gained;

    ui_show_message("Gained EXP!", "");
    ui_print_num(12, 15, gained, 4);
    ui_wait_button();

    /* Check level up */
    leveled = creature_check_levelup(winner);
    if (leveled) {
        ui_show_message(species_db[winner->species].name, "leveled up!");
        ui_print(14, 16, "Lv");
        ui_print_num(16, 16, winner->level, 2);
        ui_wait_button();

        /* Check evolution */
        {
            uint8_t old_species = winner->species;
            creature_check_evolution(winner);
            if (winner->species != old_species) {
                ui_show_message(species_db[old_species].name, "is evolving!");
                ui_wait_button();
                ui_show_message("Evolved into", species_db[winner->species].name);
                ui_wait_button();
            }
        }
    }
}

/* =========================================================
   MAIN BATTLE LOOP
   ========================================================= */
static uint8_t run_battle(Creature *wild_mon) {
    uint8_t player_faster;

    /* Initialize sides */
    player_side.mon = &save.party[0]; /* Lead creature */
    player_side.atk_stage = 0;
    player_side.def_stage = 0;
    player_side.spatk_stage = 0;
    player_side.spdef_stage = 0;
    player_side.speed_stage = 0;
    player_side.is_defending = 0;

    enemy_side.mon = wild_mon;
    enemy_side.atk_stage = 0;
    enemy_side.def_stage = 0;
    enemy_side.spatk_stage = 0;
    enemy_side.spdef_stage = 0;
    enemy_side.speed_stage = 0;
    enemy_side.is_defending = 0;

    battle_over = 0;
    battle_won = 0;

    /* Find first alive party member */
    {
        uint8_t i;
        for (i = 0; i < save.party_count; i++) {
            if (save.party[i].hp > 0) {
                player_side.mon = &save.party[i];
                break;
            }
        }
    }

    ui_fade_out();
    battle_draw_scene();
    ui_fade_in();

    /* Encounter message */
    if (is_boss_battle) {
        ui_show_message("Boss battle!", species_db[wild_mon->species].name);
    } else {
        ui_show_message("Wild", species_db[wild_mon->species].name);
    }
    ui_wait_button();

    /* Main battle loop */
    while (!battle_over) {
        /* Determine turn order by speed */
        player_faster = (player_side.mon->speed >= enemy_side.mon->speed);

        if (player_faster) {
            battle_player_turn();
            if (battle_over) break;
            if (battle_check_faint(&enemy_side)) {
                battle_won = 1;
                break;
            }

            battle_enemy_turn();
            if (battle_check_faint(&player_side)) {
                /* Try to send next creature */
                uint8_t i, found = 0;
                for (i = 0; i < save.party_count; i++) {
                    if (save.party[i].hp > 0) {
                        player_side.mon = &save.party[i];
                        player_side.atk_stage = 0;
                        player_side.def_stage = 0;
                        player_side.spatk_stage = 0;
                        player_side.spdef_stage = 0;
                        player_side.speed_stage = 0;
                        ui_show_message("Go,", species_db[player_side.mon->species].name);
                        ui_wait_button();
                        battle_draw_scene();
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    battle_won = 0;
                    break;
                }
            }
        } else {
            battle_enemy_turn();
            if (battle_check_faint(&player_side)) {
                uint8_t i, found = 0;
                for (i = 0; i < save.party_count; i++) {
                    if (save.party[i].hp > 0) {
                        player_side.mon = &save.party[i];
                        player_side.atk_stage = 0;
                        player_side.def_stage = 0;
                        player_side.spatk_stage = 0;
                        player_side.spdef_stage = 0;
                        player_side.speed_stage = 0;
                        ui_show_message("Go,", species_db[player_side.mon->species].name);
                        ui_wait_button();
                        battle_draw_scene();
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    battle_won = 0;
                    break;
                }
            }

            battle_player_turn();
            if (battle_over) break;
            if (battle_check_faint(&enemy_side)) {
                battle_won = 1;
                break;
            }
        }

        /* End-of-turn status effects */
        battle_apply_status(&player_side);
        if (battle_check_faint(&player_side)) {
            uint8_t i, found = 0;
            for (i = 0; i < save.party_count; i++) {
                if (save.party[i].hp > 0) {
                    player_side.mon = &save.party[i];
                    found = 1;
                    break;
                }
            }
            if (!found) { battle_won = 0; break; }
        }

        battle_apply_status(&enemy_side);
        if (battle_check_faint(&enemy_side)) {
            battle_won = 1;
            break;
        }

        battle_draw_hud();
    }

    /* Battle result */
    if (battle_won) {
        ui_show_message("You won!", "");
        ui_wait_button();
        battle_award_exp(player_side.mon, wild_mon);

        /* Gold reward */
        {
            uint16_t gold = (uint16_t)(wild_mon->level * 3);
            if (is_boss_battle) gold *= 3;
            save.gold += gold;
            ui_show_message("Got gold:", "");
            ui_print_num(10, 15, gold, 4);
            ui_wait_button();
        }
    } else if (!battle_over) {
        /* Total party wipe */
        ui_show_message("Blacked out!", "");
        ui_wait_button();
        /* Heal party and return to zone start */
        {
            uint8_t i;
            for (i = 0; i < save.party_count; i++) {
                creature_heal_full(&save.party[i]);
            }
        }
    }

    /* Hide battle sprites */
    move_sprite(SPR_CREATURE, 0, 0);
    move_sprite(SPR_CREATURE + 1, 0, 0);
    move_sprite(SPR_CREATURE + 2, 0, 0);
    move_sprite(SPR_CREATURE + 3, 0, 0);

    return battle_won;
}

/* Public interface */
uint8_t battle_wild(Creature *wild) {
    is_boss_battle = 0;
    return run_battle(wild);
}

uint8_t battle_boss(uint8_t species, uint8_t level) {
    Creature boss;
    creature_init(&boss, species, level);
    is_boss_battle = 1;
    return run_battle(&boss);
}
