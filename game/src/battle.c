/*  battle.c  –  Turn-based creature battle system.
 *
 *  Battle flow:
 *    1. Player picks action  (FIGHT / CATCH / SWAP / RUN)
 *    2. If FIGHT → choose a skill from unlocked nodes
 *    3. Speed comparison decides who goes first (SWIFT overrides)
 *    4. Both sides act; damage includes crits + keystone effects
 *    5. Check for KO / catch / flee
 *    6. End of turn: SP regen, REGEN keystone
 *    7. Victory → award EXP, return to overworld
 */

#include "battle.h"
#include "creature.h"
#include "skilltree.h"
#include "sprite_gen.h"
#include "ui.h"
#include "rng.h"
#include "gfx_data.h"
#include <gb/gb.h>
#include <string.h>

/* ---- Module state ----------------------------------------- */

uint8_t battle_result;

static uint8_t  bstate;
static uint8_t  menu_sel;
static uint8_t  skill_sel;
static uint8_t  swap_sel;
static uint8_t  msg_timer;

static Creature *p_crea;         /* active party creature    */
static Creature *e_crea;         /* enemy creature (external)*/

/* Skill indices available for the player creature */
static uint8_t  p_skills[MAX_ACTIVE_SKILLS];
static uint8_t  p_skill_count;

/* Defence boost flags */
static uint8_t  p_def_boost;
static uint8_t  e_def_boost;

/* Turn flow */
static uint8_t  player_first;    /* 1 = player acts before enemy */
static uint8_t  turn_phase;      /* 0 = first act done, 1 = second */

/* Battle flags */
static uint8_t  is_boss;         /* 1 = boss battle, can't flee */
static uint8_t  p_last_stand;    /* 1 = LAST_STAND still available */
static uint8_t  e_last_stand;
static uint8_t  last_was_crit;   /* for display in ACT state */
static uint8_t  cant_flee_timer; /* frames to show "CAN'T FLEE!" */

/* ---- Keystone helper -------------------------------------- */

static uint8_t has_keystone(const Creature *c, uint8_t ntype) {
    uint8_t i;
    for (i = 0; i < c->tree.count; i++) {
        if (NODE_UNLOCKED(c->tree.nodes[i]) &&
            NODE_NTYPE(c->tree.nodes[i]) == ntype)
            return 1;
    }
    return 0;
}

/* ---- Damage calculation ----------------------------------- */

static uint16_t calc_damage(Creature *attacker, Creature *defender,
                            const SkillNode *skill, uint8_t def_boosted) {
    uint16_t dmg;
    uint8_t  eff;
    uint16_t atk_stat, def_stat;

    atk_stat = attacker->atk;
    def_stat = defender->def;

    /* BERSERK: +1 ATK per 10% HP missing */
    if (has_keystone(attacker, NTYPE_BERSERK) && attacker->max_hp > 0) {
        uint8_t pct_missing = (uint8_t)(
            (attacker->max_hp - attacker->hp) * 10u / attacker->max_hp);
        atk_stat += pct_missing;
    }

    dmg = (uint16_t)(2u * attacker->level / 5u + 2u);
    dmg = dmg * skill->power;
    dmg = dmg * atk_stat;

    if (def_boosted) def_stat = def_stat * 3u / 2u;

    dmg = dmg / (def_stat * 50u);
    dmg += 2u;

    /* Type effectiveness */
    eff = type_effectiveness(skill->element, defender->type);
    if (eff == 2) {
        /* MASTERY: super effective = 2x instead of 1.5x */
        if (has_keystone(attacker, NTYPE_MASTERY))
            dmg = dmg * 2u;
        else
            dmg = dmg * 3u / 2u;
    }
    if (eff == 0) dmg = dmg / 2u;

    /* Critical hit: chance = SPD/4 out of 256 */
    last_was_crit = 0;
    {
        uint8_t crit_threshold = attacker->spd / 4u;
        if (crit_threshold < 4) crit_threshold = 4;
        if (rng_range(0, 255) < crit_threshold) {
            last_was_crit = 1;
            dmg = dmg * 3u / 2u;
        }
    }

    /* SPECIAL category: +25% damage */
    if (skill->category == SKILL_SPECIAL) {
        dmg = dmg * 5u / 4u;
    }

    /* Random variance 85-100% */
    dmg = dmg * (uint16_t)rng_range(85, 100) / 100u;

    if (dmg < 1) dmg = 1;
    return dmg;
}

/* ---- Skill application ------------------------------------ */

static void apply_skill(Creature *user, Creature *target,
                        const SkillNode *skill, uint8_t is_player) {
    uint16_t val;
    uint8_t ntype = NODE_NTYPE(*skill);

    last_was_crit = 0;

    if (skill->category == SKILL_DEFEND) {
        if (is_player) p_def_boost = 1; else e_def_boost = 1;
        return;
    }
    if (skill->category == SKILL_SUPPORT) {
        val = (uint16_t)skill->power / 2u + user->spc / 3u;
        user->hp += val;
        if (user->hp > user->max_hp) user->hp = user->max_hp;
        return;
    }

    /* ATTACK or SPECIAL: deal damage */
    val = calc_damage(user, target, skill,
                      is_player ? e_def_boost : p_def_boost);

    /* LAST_STAND: survive one lethal hit per battle */
    if (val >= target->hp) {
        uint8_t *ls = is_player ? &e_last_stand : &p_last_stand;
        if (*ls && has_keystone(target, NTYPE_LAST_STAND)) {
            target->hp = 1;
            *ls = 0;
        } else {
            target->hp = 0;
        }
    } else {
        target->hp -= val;
    }

    /* Consume defence boost (FORTRESS makes it persist) */
    if (is_player) {
        if (!has_keystone(target, NTYPE_FORTRESS)) e_def_boost = 0;
    } else {
        if (!has_keystone(target, NTYPE_FORTRESS)) p_def_boost = 0;
    }

    /* VAMPIRIC: heal 25% of damage dealt */
    if (ntype == NTYPE_VAMPIRIC && val > 0) {
        uint16_t heal = val / 4u;
        if (heal < 1) heal = 1;
        user->hp += heal;
        if (user->hp > user->max_hp) user->hp = user->max_hp;
    }

    /* DRAIN: reduce target SP */
    if (ntype == NTYPE_DRAIN) {
        uint8_t drain = skill->cost;
        if (drain > target->sp) target->sp = 0;
        else target->sp -= drain;
    }

    /* LEECH_SP: recover half SP cost */
    if (ntype == NTYPE_LEECH_SP) {
        uint8_t recover = skill->cost / 2u;
        if (recover < 1) recover = 1;
        user->sp += recover;
        if (user->sp > user->sp_max) user->sp = user->sp_max;
    }

    /* THORNS: reflect 25% damage back (can't KO attacker) */
    if (has_keystone(target, NTYPE_THORNS) && val > 0) {
        uint16_t reflect = val / 4u;
        if (reflect < 1) reflect = 1;
        if (reflect >= user->hp)
            user->hp = 1;
        else
            user->hp -= reflect;
    }
}

/* ---- Enemy AI --------------------------------------------- */

static uint8_t enemy_pick_skill(void) {
    uint8_t skills[MAX_ACTIVE_SKILLS];
    uint8_t count, i, best_idx;
    uint8_t best_score, score;
    const SkillNode *sk;

    count = skilltree_get_usable(&e_crea->tree, skills, MAX_ACTIVE_SKILLS);
    if (count == 0) return 0;

    best_idx   = 0;
    best_score = 0;

    for (i = 0; i < count; i++) {
        sk = &e_crea->tree.nodes[skills[i]];
        if (e_crea->sp < sk->cost) continue;

        score = 10;

        if (sk->category == SKILL_SUPPORT) {
            /* Heal priority when low HP */
            if (e_crea->hp * 100u / e_crea->max_hp < 30u)
                score = 80;
            else if (e_crea->hp * 100u / e_crea->max_hp < 60u)
                score = 30;
            else
                score = 5;
        } else if (sk->category == SKILL_ATTACK || sk->category == SKILL_SPECIAL) {
            /* Prefer super-effective attacks */
            uint8_t eff = type_effectiveness(sk->element, p_crea->type);
            if (eff == 2) score = 55 + sk->power / 3u;
            else if (eff == 1) score = 30 + sk->power / 4u;
            else score = 10;
            /* Bonus for high power */
            score += sk->power / 10u;
            /* Finish off: prefer attacks when target is low */
            if (p_crea->hp * 100u / p_crea->max_hp < 25u)
                score += 20;
        } else if (sk->category == SKILL_DEFEND) {
            if (e_crea->hp * 100u / e_crea->max_hp > 60u)
                score = 20;
            else
                score = 8;
        }

        /* Add randomness so it's not fully predictable */
        score += rng_range(0, 25);

        if (score > best_score) {
            best_score = score;
            best_idx = skills[i];
        }
    }

    if (best_score == 0) return 0;
    return best_idx;
}

/* ---- Initiative ------------------------------------------- */

static void determine_initiative(void) {
    uint8_t p_swift = has_keystone(p_crea, NTYPE_SWIFT);
    uint8_t e_swift = has_keystone(e_crea, NTYPE_SWIFT);

    if (p_swift && !e_swift)
        player_first = 1;
    else if (e_swift && !p_swift)
        player_first = 0;
    else if (p_crea->spd > e_crea->spd)
        player_first = 1;
    else if (p_crea->spd < e_crea->spd)
        player_first = 0;
    else
        player_first = rng_range(0, 1);
}

/* ---- End-of-turn effects ---------------------------------- */

static void end_of_turn_effects(void) {
    /* SP recovery: +1 per turn */
    if (p_crea->sp < p_crea->sp_max) p_crea->sp++;
    if (e_crea->sp < e_crea->sp_max) e_crea->sp++;

    /* REGEN: heal 5% max HP per turn */
    if (has_keystone(p_crea, NTYPE_REGEN)) {
        uint16_t heal = p_crea->max_hp / 20u;
        if (heal < 1) heal = 1;
        p_crea->hp += heal;
        if (p_crea->hp > p_crea->max_hp) p_crea->hp = p_crea->max_hp;
    }
    if (has_keystone(e_crea, NTYPE_REGEN)) {
        uint16_t heal = e_crea->max_hp / 20u;
        if (heal < 1) heal = 1;
        e_crea->hp += heal;
        if (e_crea->hp > e_crea->max_hp) e_crea->hp = e_crea->max_hp;
    }
}

/* ---- Public API ------------------------------------------- */

void battle_start(Creature *enemy, uint8_t boss_flag) {
    bstate   = BSTATE_INIT;
    e_crea   = enemy;
    p_crea   = &party[0];
    menu_sel = 0;
    skill_sel = 0;
    swap_sel  = 0;
    msg_timer = 0;
    is_boss   = boss_flag;
    player_first = 1;
    turn_phase   = 0;
    p_def_boost  = 0;
    e_def_boost  = 0;
    last_was_crit = 0;
    cant_flee_timer = 0;
    battle_result = BATTLE_RESULT_NONE;

    /* LAST_STAND: available once per battle */
    p_last_stand = has_keystone(p_crea, NTYPE_LAST_STAND);
    e_last_stand = has_keystone(e_crea, NTYPE_LAST_STAND);

    /* Reset battle SP to full */
    p_crea->sp = p_crea->sp_max;
    e_crea->sp = e_crea->sp_max;

    /* Gather player skills */
    p_skill_count = skilltree_get_usable(&p_crea->tree, p_skills,
                                         MAX_ACTIVE_SKILLS);

    /* Generate and load procedural sprites */
    set_bkg_data(TILE_CREA_BASE, CREA_SPRITE_TILES,
                 sprite_gen_build(e_crea));
    set_bkg_data(TILE_CREA_BASE + CREA_SPRITE_TILES, CREA_SPRITE_TILES,
                 sprite_gen_build(p_crea));

    /* Palette attributes for creature sprite areas */
    {
        static const uint8_t type_pal[] = { 5, 4, 3, 7, 6, 7 };
        ui_set_palette_rect(14, 2, 4, 4, type_pal[e_crea->type]);
        ui_set_palette_rect(2, 8, 4, 4, type_pal[p_crea->type]);
    }
}

uint8_t battle_update(void) {

    switch (bstate) {

    /* ---- Initialisation ------------------------------------ */
    case BSTATE_INIT:
        msg_timer = 30;
        bstate = BSTATE_PLAYER_MENU;
        break;

    /* ---- Player action menu -------------------------------- */
    case BSTATE_PLAYER_MENU:
        if (msg_timer) { msg_timer--; break; }
        if (cant_flee_timer > 0) cant_flee_timer--;

        if (PRESSED(J_UP)   || PRESSED(J_DOWN))  menu_sel ^= 1;
        if (PRESSED(J_LEFT) || PRESSED(J_RIGHT)) menu_sel ^= 2;
        if (menu_sel > 3) menu_sel = 0;

        if (PRESSED(J_A)) {
            switch (menu_sel) {
                case 0: /* FIGHT */
                    skill_sel = 0;
                    bstate = BSTATE_SELECT_SKILL;
                    break;
                case 1: /* CATCH */
                    bstate = BSTATE_CATCH_TRY;
                    break;
                case 2: /* SWAP */
                    if (party_count > 1) {
                        swap_sel = 0;
                        bstate = BSTATE_SWAP;
                    }
                    break;
                case 3: /* RUN */
                    if (is_boss) {
                        cant_flee_timer = 40;
                    } else {
                        bstate = BSTATE_RUN;
                    }
                    break;
            }
        }
        break;

    /* ---- Skill selection ----------------------------------- */
    case BSTATE_SELECT_SKILL:
        if (PRESSED(J_UP)   && skill_sel > 0) skill_sel--;
        if (PRESSED(J_DOWN) && p_skill_count > 0 && skill_sel < p_skill_count - 1) skill_sel++;

        if (PRESSED(J_B)) {
            bstate = BSTATE_PLAYER_MENU;
            break;
        }
        if (PRESSED(J_A)) {
            if (p_crea->sp >= p_crea->tree.nodes[p_skills[skill_sel]].cost) {
                /* Determine turn order by speed */
                determine_initiative();
                turn_phase = 0;
                bstate = player_first ? BSTATE_PLAYER_ACT : BSTATE_ENEMY_ACT;
            }
        }
        break;

    /* ---- Player acts --------------------------------------- */
    case BSTATE_PLAYER_ACT: {
        const SkillNode *sk = &p_crea->tree.nodes[p_skills[skill_sel]];
        p_crea->sp -= sk->cost;
        apply_skill(p_crea, e_crea, sk, 1);
        /* MULTICAST: 30% chance to act again */
        if (NODE_NTYPE(*sk) == NTYPE_MULTICAST && rng_range(0, 99) < 30) {
            apply_skill(p_crea, e_crea, sk, 1);
        }
        msg_timer   = 30;
        bstate = BSTATE_CHECK;
        break;
    }

    /* ---- Enemy acts ---------------------------------------- */
    case BSTATE_ENEMY_ACT: {
        uint8_t eidx = enemy_pick_skill();
        const SkillNode *sk = &e_crea->tree.nodes[eidx];
        if (e_crea->sp >= sk->cost) e_crea->sp -= sk->cost;
        apply_skill(e_crea, p_crea, sk, 0);
        if (NODE_NTYPE(*sk) == NTYPE_MULTICAST && rng_range(0, 99) < 30) {
            apply_skill(e_crea, p_crea, sk, 0);
        }
        msg_timer   = 30;
        bstate = BSTATE_CHECK;
        break;
    }

    /* ---- Check HP / advance turn phase -------------------- */
    case BSTATE_CHECK:
        if (msg_timer) { msg_timer--; break; }

        if (e_crea->hp == 0) { bstate = BSTATE_VICTORY; break; }
        if (p_crea->hp == 0) { bstate = BSTATE_DEFEAT;  break; }

        if (turn_phase == 0) {
            /* First actor done → second actor goes */
            turn_phase = 1;
            bstate = player_first ? BSTATE_ENEMY_ACT : BSTATE_PLAYER_ACT;
        } else {
            /* Both acted → end of turn */
            end_of_turn_effects();
            bstate = BSTATE_PLAYER_MENU;
        }
        break;

    /* ---- Victory ------------------------------------------- */
    case BSTATE_VICTORY:
        if (msg_timer) { msg_timer--; break; }
        if (battle_result == BATTLE_RESULT_NONE) {
            creature_gain_exp(p_crea,
                              20u + (uint16_t)e_crea->level * 4u);
            battles_won++;
            battle_result = BATTLE_RESULT_WIN;
            msg_timer = 60;
            break;
        }
        if (PRESSED(J_A) || PRESSED(J_B)) {
            bstate = BSTATE_DONE;
        }
        break;

    /* ---- Defeat -------------------------------------------- */
    case BSTATE_DEFEAT:
        if (msg_timer) { msg_timer--; break; }
        if (battle_result == BATTLE_RESULT_NONE) {
            battle_result = BATTLE_RESULT_LOSE;
            msg_timer = 60;
            break;
        }
        if (PRESSED(J_A) || PRESSED(J_B)) {
            p_crea->hp = 1;
            bstate = BSTATE_DONE;
        }
        break;

    /* ---- Catch attempt ------------------------------------- */
    case BSTATE_CATCH_TRY:
        if (msg_timer) { msg_timer--; break; }
        if (battle_result == BATTLE_RESULT_NONE) {
            uint16_t chance;
            chance = (uint16_t)species_table[e_crea->species].catch_rate;
            chance = chance * (e_crea->max_hp * 2u - e_crea->hp);
            chance = chance / (e_crea->max_hp * 2u);
            if (chance < 10) chance = 10;

            if (rng_range(0, 255) < (uint8_t)chance && party_count < MAX_PARTY) {
                memcpy(&party[party_count], e_crea, sizeof(Creature));
                party_count++;
                total_catches++;
                battle_result = BATTLE_RESULT_CATCH;
                msg_timer = 60;
            } else {
                msg_timer = 30;
                turn_phase = 1;  /* enemy gets free attack */
                bstate = BSTATE_ENEMY_ACT;
            }
            break;
        }
        if (PRESSED(J_A) || PRESSED(J_B)) {
            bstate = BSTATE_DONE;
        }
        break;

    /* ---- Run away ------------------------------------------ */
    case BSTATE_RUN:
        if (msg_timer) { msg_timer--; break; }
        if (battle_result == BATTLE_RESULT_NONE) {
            if (p_crea->spd >= e_crea->spd || rng_range(0, 3) > 0) {
                battle_result = BATTLE_RESULT_RUN;
                msg_timer = 30;
            } else {
                msg_timer = 30;
                turn_phase = 1;
                bstate = BSTATE_ENEMY_ACT;
            }
            break;
        }
        if (PRESSED(J_A) || PRESSED(J_B)) {
            bstate = BSTATE_DONE;
        }
        break;

    /* ---- Party swap ---------------------------------------- */
    case BSTATE_SWAP:
        if (PRESSED(J_UP)   && swap_sel > 0) swap_sel--;
        if (PRESSED(J_DOWN) && swap_sel < party_count - 1) swap_sel++;

        if (PRESSED(J_B)) {
            bstate = BSTATE_PLAYER_MENU;
            break;
        }
        if (PRESSED(J_A)) {
            if (swap_sel != 0 && party[swap_sel].hp > 0) {
                /* Swap lead with selected */
                Creature tmp;
                memcpy(&tmp, &party[0], sizeof(Creature));
                memcpy(&party[0], &party[swap_sel], sizeof(Creature));
                memcpy(&party[swap_sel], &tmp, sizeof(Creature));

                /* Update battle state for new lead */
                p_crea = &party[0];
                p_skill_count = skilltree_get_usable(&p_crea->tree, p_skills,
                                                     MAX_ACTIVE_SKILLS);
                p_crea->sp = p_crea->sp_max;
                p_def_boost = 0;
                p_last_stand = has_keystone(p_crea, NTYPE_LAST_STAND);

                /* Reload player sprite */
                set_bkg_data(TILE_CREA_BASE + CREA_SPRITE_TILES,
                             CREA_SPRITE_TILES,
                             sprite_gen_build(p_crea));

                msg_timer = 20;
                turn_phase = 1;  /* enemy gets free attack after swap */
                bstate = BSTATE_ENEMY_ACT;
            }
        }
        break;

    /* ---- Done – signal main to leave battle state ---------- */
    case BSTATE_DONE:
        return 1;
    }

    return 0;
}

/* ---- Rendering (delegates to ui.c helpers) ---------------- */

void battle_render(void) {
    char buf[SKILL_NAME_LEN];
    uint8_t i;

    /* Clear screen */
    ui_clear();

    /* Enemy info – top */
    ui_print(1, 0, e_crea->name);
    ui_print(12, 0, "LV");
    ui_print_num(14, 0, e_crea->level);
    ui_draw_hp_bar(1, 1, e_crea->hp, e_crea->max_hp);

    /* Player info – middle */
    ui_print(1, 6, p_crea->name);
    ui_print(12, 6, "LV");
    ui_print_num(14, 6, p_crea->level);
    ui_draw_hp_bar(1, 7, p_crea->hp, p_crea->max_hp);
    /* SP display */
    ui_print(1, 8, "SP");
    ui_print_num(3, 8, p_crea->sp);
    ui_print(5, 8, "/");
    ui_print_num(6, 8, p_crea->sp_max);

    /* Draw creature type badges */
    ui_print(1, 2, type_names[e_crea->type]);
    ui_print(1, 9, type_names[p_crea->type]);

    /* Draw creature sprites as 4x4 background tiles */
    {
        uint8_t row_tiles[4];
        uint8_t r;
        /* Enemy sprite */
        for (r = 0; r < 4; r++) {
            row_tiles[0] = TILE_CREA_BASE + r * 4;
            row_tiles[1] = TILE_CREA_BASE + r * 4 + 1;
            row_tiles[2] = TILE_CREA_BASE + r * 4 + 2;
            row_tiles[3] = TILE_CREA_BASE + r * 4 + 3;
            set_bkg_tiles(14, 2 + r, 4, 1, row_tiles);
        }
        /* Player sprite */
        for (r = 0; r < 4; r++) {
            row_tiles[0] = TILE_CREA_BASE + CREA_SPRITE_TILES + r * 4;
            row_tiles[1] = TILE_CREA_BASE + CREA_SPRITE_TILES + r * 4 + 1;
            row_tiles[2] = TILE_CREA_BASE + CREA_SPRITE_TILES + r * 4 + 2;
            row_tiles[3] = TILE_CREA_BASE + CREA_SPRITE_TILES + r * 4 + 3;
            set_bkg_tiles(2, 8 + r, 4, 1, row_tiles);
        }
    }

    /* Re-apply creature palette attributes (cleared by ui_clear) */
    {
        static const uint8_t type_pal[] = { 5, 4, 3, 7, 6, 7 };
        ui_set_palette_rect(14, 2, 4, 4, type_pal[e_crea->type]);
        ui_set_palette_rect(2, 8, 4, 4, type_pal[p_crea->type]);
    }

    /* ---- State-dependent lower section -------------------- */
    switch (bstate) {

    case BSTATE_PLAYER_MENU:
        ui_draw_box(0, 12, 20, 6);
        ui_print(2,  13, "FIGHT");
        ui_print(11, 13, "CATCH");
        ui_print(2,  15, "SWAP");
        ui_print(11, 15, "RUN");
        /* Selection arrow */
        ui_print((menu_sel & 2) ? 10 : 1,
                 (menu_sel & 1) ? 15 : 13,
                 ">");
        /* "Can't flee" message for bosses */
        if (cant_flee_timer > 0) {
            ui_print(2, 16, "CANT FLEE!");
        }
        break;

    case BSTATE_SELECT_SKILL: {
        uint8_t scroll_top = 0;
        ui_draw_box(0, 10, 20, 8);
        ui_print(1, 10, "PICK SKILL");
        if (skill_sel >= 4) scroll_top = skill_sel - 3;
        for (i = 0; i < 4; i++) {
            uint8_t si = scroll_top + i;
            if (si >= p_skill_count) break;
            {
                const SkillNode *sk = &p_crea->tree.nodes[p_skills[si]];
                skilltree_skill_name(buf, sk->element, sk->category,
                                     p_skills[si] & 3);
                ui_print(3, 12 + i, buf);
                /* Show keystone marker for enhanced actives */
                if (NODE_NTYPE(*sk) != NTYPE_NORMAL)
                    ui_print(14, 12 + i, "!");
                ui_print_num(15, 12 + i, sk->power);
                ui_print(17, 12 + i, skilltree_cat_tag(sk->category));
            }
        }
        if (p_skill_count > 0) {
            ui_print(1, 12 + (skill_sel - scroll_top), ">");
        }
        break;
    }

    case BSTATE_SWAP:
        ui_draw_box(0, 10, 20, 8);
        ui_print(1, 10, "SWAP TO");
        for (i = 0; i < party_count && i < 4; i++) {
            Creature *c = &party[i];
            uint8_t y = 12 + i;
            if (i == 0) {
                ui_print(3, y, c->name);
                ui_print(13, y, "(CUR)");
            } else if (c->hp == 0) {
                ui_print(3, y, c->name);
                ui_print(13, y, "FAINT");
            } else {
                ui_print(3, y, c->name);
                ui_print(13, y, "LV");
                ui_print_num(15, y, c->level);
            }
        }
        ui_print(1, 12 + swap_sel, ">");
        break;

    case BSTATE_VICTORY:
        ui_draw_box(0, 12, 20, 6);
        ui_print(2, 13, "YOU WIN!");
        ui_print(2, 15, "EXP +");
        ui_print_num(7, 15, 20u + (uint16_t)e_crea->level * 4u);
        break;

    case BSTATE_DEFEAT:
        ui_draw_box(0, 12, 20, 6);
        ui_print(2, 13, "DEFEATED...");
        ui_print(2, 15, "PRESS A");
        break;

    case BSTATE_CATCH_TRY:
        ui_draw_box(0, 12, 20, 6);
        if (battle_result == BATTLE_RESULT_CATCH) {
            ui_print(2, 13, "CAUGHT ");
            ui_print(9, 13, e_crea->name);
        } else {
            ui_print(2, 13, "CATCHING...");
        }
        break;

    case BSTATE_RUN:
        ui_draw_box(0, 12, 20, 6);
        if (battle_result == BATTLE_RESULT_RUN) {
            ui_print(2, 13, "GOT AWAY!");
        } else {
            ui_print(2, 13, "CANT ESCAPE!");
        }
        break;

    default:
        /* INIT / CHECK / ACT states: show a simple message box */
        ui_draw_box(0, 14, 20, 4);
        if (bstate == BSTATE_PLAYER_ACT || bstate == BSTATE_ENEMY_ACT) {
            ui_print(2, 15, bstate == BSTATE_PLAYER_ACT ?
                     p_crea->name : e_crea->name);
            ui_print(11, 15, "ATTACKS!");
        }
        if (last_was_crit) {
            ui_print(2, 16, "CRITICAL!");
        }
        break;
    }
}
