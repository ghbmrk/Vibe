/*  battle.c  –  Turn-based creature battle system.
 *
 *  Battle flow:
 *    1. Player picks action  (FIGHT / CATCH / SKILLS / RUN)
 *    2. If FIGHT → choose a skill from unlocked nodes
 *    3. Speed decides who goes first
 *    4. Both sides act, damage / heal resolved
 *    5. Check for KO / catch / flee
 *    6. Victory → award EXP, return to overworld
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

static uint8_t  bstate;          /* current battle sub-state */
static uint8_t  menu_sel;        /* selected menu item       */
static uint8_t  skill_sel;       /* selected skill index     */
static uint8_t  msg_timer;       /* frames to show a message */
static uint8_t  player_acted;

static Creature *p_crea;         /* active party creature    */
static Creature *e_crea;         /* enemy creature (external)*/

/* Skill indices available for the player creature */
static uint8_t  p_skills[MAX_ACTIVE_SKILLS];
static uint8_t  p_skill_count;

/* Temporary defence boost flag (lasts one turn) */
static uint8_t  p_def_boost;
static uint8_t  e_def_boost;

/* ---- Helpers ---------------------------------------------- */

/* Simplified damage formula:
 *   base = (2*level/5 + 2) * power * atk / (def * 50) + 2
 *   apply type effectiveness multiplier
 *   apply small random variance 85-100 %
 */
static uint16_t calc_damage(Creature *attacker, Creature *defender,
                            const SkillNode *skill, uint8_t def_boosted) {
    uint16_t dmg;
    uint8_t  eff;
    uint16_t def_stat;

    dmg = (uint16_t)(2u * attacker->level / 5u + 2u);
    dmg = dmg * skill->power;
    dmg = dmg * attacker->atk;

    def_stat = defender->def;
    if (def_boosted) def_stat = def_stat * 3u / 2u;   /* +50 % DEF */

    dmg = dmg / (def_stat * 50u);
    dmg += 2u;

    /* Type effectiveness */
    eff = type_effectiveness(skill->element, defender->type);
    if (eff == 2) dmg = dmg * 3u / 2u;   /* super effective */
    if (eff == 0) dmg = dmg / 2u;         /* resisted        */

    /* Small random variance */
    dmg = dmg * (uint16_t)rng_range(85, 100) / 100u;

    if (dmg < 1) dmg = 1;
    return dmg;
}

static void apply_skill(Creature *user, Creature *target,
                        const SkillNode *skill, uint8_t is_player) {
    uint16_t val;

    if (skill->category == SKILL_DEFEND) {
        /* Raise own DEF for next incoming attack */
        if (is_player) p_def_boost = 1; else e_def_boost = 1;
        return;
    }
    if (skill->category == SKILL_SUPPORT) {
        /* Heal */
        val = (uint16_t)skill->power / 2u + user->spc / 3u;
        user->hp += val;
        if (user->hp > user->max_hp) user->hp = user->max_hp;
        return;
    }

    /* ATTACK or SPECIAL */
    val = calc_damage(user, target, skill, is_player ? e_def_boost : p_def_boost);
    if (skill->category == SKILL_SPECIAL) {
        val = val * 5u / 4u;  /* +25 % for special skills */
    }
    if (val >= target->hp) {
        target->hp = 0;
    } else {
        target->hp -= val;
    }

    /* Consume defence boost after being hit */
    if (is_player) e_def_boost = 0; else p_def_boost = 0;
}

/* AI: enemy picks a random unlocked skill it can afford */
static uint8_t enemy_pick_skill(void) {
    uint8_t skills[MAX_ACTIVE_SKILLS];
    uint8_t count, i, tries;

    count = skilltree_get_usable(&e_crea->tree, skills, MAX_ACTIVE_SKILLS);
    if (count == 0) return 0;  /* root / fallback */

    /* Try to find a skill the enemy can afford */
    for (tries = 0; tries < 8; tries++) {
        i = rng_range(0, count - 1);
        if (e_crea->sp >= e_crea->tree.nodes[skills[i]].cost) {
            return skills[i];
        }
    }
    /* Fall back to root (cheapest) */
    return 0;
}

/* ---- Public API ------------------------------------------- */

void battle_start(Creature *enemy) {
    bstate   = BSTATE_INIT;
    e_crea   = enemy;
    p_crea   = &party[0];  /* lead creature fights */
    menu_sel = 0;
    skill_sel = 0;
    msg_timer = 0;
    player_acted = 0;
    p_def_boost  = 0;
    e_def_boost  = 0;
    battle_result = BATTLE_RESULT_NONE;

    /* Reset battle SP to full */
    p_crea->sp = p_crea->sp_max;
    e_crea->sp = e_crea->sp_max;

    /* Gather player skills */
    p_skill_count = skilltree_get_usable(&p_crea->tree, p_skills,
                                         MAX_ACTIVE_SKILLS);

    /* Generate and load procedural sprites into VRAM.
     * Each creature's sprite reflects its skill tree state. */
    set_bkg_data(TILE_CREA_BASE, CREA_SPRITE_TILES,
                 sprite_gen_build(e_crea));
    set_bkg_data(TILE_CREA_BASE + CREA_SPRITE_TILES, CREA_SPRITE_TILES,
                 sprite_gen_build(p_crea));

    /* Set palette attributes for creature sprite areas.
     * Map element type to BG palette:
     * FLAME→5, AQUA→4, TERRA→3, VOLT→7, SHADOW→6, AETHER→7 */
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
                case 2: /* SKILLS (opens skill tree – handled by main) */
                    game_state = STATE_SKILLTREE;
                    break;
                case 3: /* RUN */
                    bstate = BSTATE_RUN;
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
            /* Check SP */
            if (p_crea->sp >= p_crea->tree.nodes[p_skills[skill_sel]].cost) {
                bstate = BSTATE_PLAYER_ACT;
            }
            /* else: not enough SP – stay on menu (could flash) */
        }
        break;

    /* ---- Player acts --------------------------------------- */
    case BSTATE_PLAYER_ACT: {
        const SkillNode *sk = &p_crea->tree.nodes[p_skills[skill_sel]];
        p_crea->sp -= sk->cost;
        apply_skill(p_crea, e_crea, sk, 1);
        msg_timer   = 30;
        player_acted = 1;
        bstate = BSTATE_CHECK;
        break;
    }

    /* ---- Enemy acts ---------------------------------------- */
    case BSTATE_ENEMY_ACT: {
        uint8_t eidx = enemy_pick_skill();
        const SkillNode *sk = &e_crea->tree.nodes[eidx];
        if (e_crea->sp >= sk->cost) e_crea->sp -= sk->cost;
        apply_skill(e_crea, p_crea, sk, 0);
        msg_timer   = 30;
        player_acted = 0;
        bstate = BSTATE_CHECK;
        break;
    }

    /* ---- Check HP / switch turns --------------------------- */
    case BSTATE_CHECK:
        if (msg_timer) { msg_timer--; break; }

        if (e_crea->hp == 0) { bstate = BSTATE_VICTORY; break; }
        if (p_crea->hp == 0) { bstate = BSTATE_DEFEAT;  break; }

        /* Alternate turns – if player just went, enemy goes next */
        if (player_acted) {
            bstate = BSTATE_ENEMY_ACT;
        } else {
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
                bstate = BSTATE_ENEMY_ACT;
            }
            break;
        }
        if (PRESSED(J_A) || PRESSED(J_B)) {
            bstate = BSTATE_DONE;
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
    char buf[NAME_LEN];
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

    /* Draw creature sprites as 4x4 background tiles.
     * Enemy: top-right area (cols 14-17, rows 2-5)
     * Player: mid-left area (cols 2-5, rows 6-9) -- overlaps info, kept compact */
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
        ui_print(2,  15, "SKILLS");
        ui_print(11, 15, "RUN");
        /* Selection arrow */
        ui_print((menu_sel & 2) ? 10 : 1,
                 (menu_sel & 1) ? 15 : 13,
                 ">");
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
                ui_print_num(15, 12 + i, sk->power);
                ui_print(17, 12 + i, skilltree_cat_tag(sk->category));
            }
        }
        if (p_skill_count > 0) {
            ui_print(1, 12 + (skill_sel - scroll_top), ">");
        }
        break;
    }

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
        break;
    }
}
