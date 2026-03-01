#include "creature.h"
#include "rng.h"

/* =========================================================
   MOVE DATABASE - 32 moves
   {name, type, category, power, accuracy, pp, effect, effect%}
   ========================================================= */
const MoveData move_db[NUM_MOVES] = {
    /* 0  Normal moves */
    {"Tackle",      TYPE_NORMAL, CAT_PHYSICAL, 40,  100, 35, STATUS_NONE, 0},
    {"Slam",        TYPE_NORMAL, CAT_PHYSICAL, 65,  90,  20, STATUS_NONE, 0},
    {"Swift",       TYPE_NORMAL, CAT_SPECIAL,  60,  100, 20, STATUS_NONE, 0},
    {"Hyper Rush",  TYPE_NORMAL, CAT_PHYSICAL, 90,  85,  10, STATUS_NONE, 0},
    /* 4  Fire moves */
    {"Ember",       TYPE_FIRE,   CAT_SPECIAL,  40,  100, 25, STATUS_BURN, 10},
    {"Fire Fang",   TYPE_FIRE,   CAT_PHYSICAL, 65,  95,  15, STATUS_BURN, 20},
    {"Inferno",     TYPE_FIRE,   CAT_SPECIAL,  90,  80,  10, STATUS_BURN, 30},
    {"Blaze Kick",  TYPE_FIRE,   CAT_PHYSICAL, 75,  90,  15, STATUS_BURN, 10},
    /* 8  Water moves */
    {"Splash",      TYPE_WATER,  CAT_SPECIAL,  40,  100, 25, STATUS_NONE, 0},
    {"Aqua Jet",    TYPE_WATER,  CAT_PHYSICAL, 55,  100, 20, STATUS_NONE, 0},
    {"Tidal Wave",  TYPE_WATER,  CAT_SPECIAL,  85,  85,  10, STATUS_NONE, 0},
    {"Hydro Pump",  TYPE_WATER,  CAT_SPECIAL,  95,  75,  5,  STATUS_NONE, 0},
    /* 12 Leaf moves */
    {"Vine Whip",   TYPE_LEAF,   CAT_PHYSICAL, 45,  100, 25, STATUS_NONE, 0},
    {"Razor Leaf",  TYPE_LEAF,   CAT_PHYSICAL, 60,  95,  20, STATUS_NONE, 0},
    {"Spore Bomb",  TYPE_LEAF,   CAT_SPECIAL,  75,  90,  15, STATUS_POISON, 20},
    {"Solar Beam",  TYPE_LEAF,   CAT_SPECIAL,  95,  80,  5,  STATUS_NONE, 0},
    /* 16 Bolt moves */
    {"Spark",       TYPE_BOLT,   CAT_SPECIAL,  40,  100, 25, STATUS_PARALYZE, 10},
    {"Thunder Fang",TYPE_BOLT,   CAT_PHYSICAL, 65,  95,  15, STATUS_PARALYZE, 20},
    {"Discharge",   TYPE_BOLT,   CAT_SPECIAL,  80,  90,  10, STATUS_PARALYZE, 15},
    {"Lightning",   TYPE_BOLT,   CAT_SPECIAL,  95,  70,  5,  STATUS_PARALYZE, 30},
    /* 20 Stone moves */
    {"Rock Throw",  TYPE_STONE,  CAT_PHYSICAL, 50,  90,  25, STATUS_NONE, 0},
    {"Stone Edge",  TYPE_STONE,  CAT_PHYSICAL, 80,  85,  10, STATUS_NONE, 0},
    {"Earthquake",  TYPE_STONE,  CAT_PHYSICAL, 90,  90,  10, STATUS_NONE, 0},
    {"Rock Slide",  TYPE_STONE,  CAT_PHYSICAL, 70,  90,  15, STATUS_NONE, 0},
    /* 24 Shadow moves */
    {"Shadow Claw", TYPE_SHADOW, CAT_PHYSICAL, 55,  100, 20, STATUS_NONE, 0},
    {"Dark Pulse",  TYPE_SHADOW, CAT_SPECIAL,  70,  90,  15, STATUS_NONE, 0},
    {"Nightmare",   TYPE_SHADOW, CAT_SPECIAL,  85,  85,  10, STATUS_SLEEP, 20},
    {"Void Rend",   TYPE_SHADOW, CAT_PHYSICAL, 90,  80,  5,  STATUS_NONE, 0},
    /* 28 Light moves */
    {"Flash",       TYPE_LIGHT,  CAT_SPECIAL,  45,  100, 25, STATUS_NONE, 0},
    {"Holy Beam",   TYPE_LIGHT,  CAT_SPECIAL,  70,  90,  15, STATUS_NONE, 0},
    {"Radiance",    TYPE_LIGHT,  CAT_SPECIAL,  85,  85,  10, STATUS_BURN, 10},
    {"Judgement",   TYPE_LIGHT,  CAT_SPECIAL,  95,  75,  5,  STATUS_NONE, 0},
};

/* =========================================================
   SPECIES DATABASE - 12 original creatures
   {name, type1, type2, hp, atk, def, spatk, spdef, spd,
    learn_moves[6], learn_levels[6], evolve_into, evolve_lv, sprite}
   ========================================================= */
const SpeciesData species_db[NUM_SPECIES] = {
    /* 0: Embrix - Fire starter */
    {"Embrix",   TYPE_FIRE,   TYPE_FIRE,    45, 55, 40, 60, 45, 50,
     {0, 4, 5, 7, 6, 3},  {1, 5, 10, 16, 22, 28},  7, 25, 0},
    /* 1: Tidalin - Water starter */
    {"Tidalin",  TYPE_WATER,  TYPE_WATER,   50, 45, 50, 55, 55, 40,
     {0, 8, 9, 10, 11, 2}, {1, 5, 10, 16, 22, 28}, 8, 25, 1},
    /* 2: Thornyx - Leaf starter */
    {"Thornyx",  TYPE_LEAF,   TYPE_LEAF,    48, 50, 55, 50, 50, 42,
     {0, 12, 13, 14, 15, 1},{1, 5, 10, 16, 22, 28}, 9, 25, 2},
    /* 3: Zappix - Electric rodent */
    {"Zappix",   TYPE_BOLT,   TYPE_BOLT,    40, 45, 35, 65, 40, 70,
     {0, 16, 17, 18, 19, 2},{1, 5, 10, 16, 22, 28}, 0xFF, 0, 3},
    /* 4: Petrock - Stone golem */
    {"Petrock",  TYPE_STONE,  TYPE_STONE,   55, 65, 70, 30, 50, 25,
     {0, 20, 23, 21, 22, 1},{1, 5, 10, 16, 22, 28}, 0xFF, 0, 4},
    /* 5: Glimmow - Light moth */
    {"Glimmow",  TYPE_LIGHT,  TYPE_LIGHT,   42, 35, 40, 60, 55, 65,
     {0, 28, 29, 30, 31, 2},{1, 5, 10, 16, 22, 28}, 0xFF, 0, 5},
    /* 6: Duskul - Shadow ghost */
    {"Duskul",   TYPE_SHADOW, TYPE_SHADOW,  48, 55, 40, 60, 45, 50,
     {0, 24, 25, 26, 27, 1},{1, 5, 10, 16, 22, 28}, 0xFF, 0, 6},
    /* 7: Blazeon - Fire/Light phoenix (Embrix evo) */
    {"Blazeon",  TYPE_FIRE,   TYPE_LIGHT,   65, 70, 55, 80, 60, 65,
     {0, 4, 7, 6, 30, 31}, {1, 5, 10, 16, 22, 28}, 0xFF, 0, 7},
    /* 8: Tsunamaw - Water dragon (Tidalin evo) */
    {"Tsunamaw", TYPE_WATER,  TYPE_STONE,   70, 65, 70, 75, 65, 50,
     {0, 8, 10, 11, 22, 21},{1, 5, 10, 16, 22, 28}, 0xFF, 0, 8},
    /* 9: Verdrath - Leaf treant (Thornyx evo) */
    {"Verdrath", TYPE_LEAF,   TYPE_STONE,   68, 70, 75, 65, 60, 45,
     {0, 12, 14, 15, 23, 22},{1, 5, 10, 16, 22, 28}, 0xFF, 0, 9},
    /* 10: Nocthrall - Shadow/Normal wraith */
    {"Nocthrall",TYPE_SHADOW, TYPE_NORMAL,  55, 70, 50, 70, 50, 60,
     {0, 24, 25, 27, 3, 26},{1, 5, 10, 16, 22, 28}, 0xFF, 0, 10},
    /* 11: Luminax - Light/Bolt celestial */
    {"Luminax",  TYPE_LIGHT,  TYPE_BOLT,    60, 45, 50, 80, 70, 60,
     {0, 28, 29, 31, 19, 18},{1, 5, 10, 16, 22, 28}, 0xFF, 0, 11},
};

/* =========================================================
   TYPE EFFECTIVENESS CHART
   Values: 0=immune, 5=resist(0.5x), 10=neutral(1x), 20=super(2x)
   [attack_type][defend_type]
   ========================================================= */
const uint8_t type_chart[NUM_TYPES][NUM_TYPES] = {
    /*              NRM  FIR  WTR  LEF  BLT  STN  SHD  LGT */
    /* NORMAL */ {  10,  10,  10,  10,  10,  10,   0,  10 },
    /* FIRE   */ {  10,   5,   5,  20,  10,  10,  10,   5 },
    /* WATER  */ {  10,  20,   5,   5,  20,  20,  10,  10 },
    /* LEAF   */ {  10,   5,  20,   5,  10,  20,  10,  10 },
    /* BOLT   */ {  10,  10,  20,   5,   5,   0,  10,  10 },
    /* STONE  */ {  10,  20,   5,   5,  20,  10,  10,  10 },
    /* SHADOW */ {   0,  10,  10,  10,  10,  10,  20,  20 },
    /* LIGHT  */ {  10,   5,  10,  10,  10,  10,  20,   5 },
};

/* =========================================================
   ZONE DATABASE - 6 zones
   ========================================================= */
const ZoneData zone_db[NUM_ZONES] = {
    /* Zone 0: Greenmeadow */
    {"Greenmeadow",  {0, 2, 3, 5},  2,  6,  1, 1,  3,  8},
    /* Zone 1: Coral Coast */
    {"Coral Coast",  {1, 3, 5, 4},  5,  12, 2, 1,  4,  15},
    /* Zone 2: Ember Peaks */
    {"Ember Peaks",  {0, 4, 6, 3},  10, 18, 3, 1,  0,  22},
    /* Zone 3: Deepwood */
    {"Deepwood",     {2, 6, 5, 10}, 15, 24, 4, 1,  6,  28},
    /* Zone 4: Stormridge */
    {"Stormridge",   {3, 4, 10, 11},20, 32, 5, 0,  10, 35},
    /* Zone 5: Shadow Spire */
    {"Shadow Spire", {6, 10, 11, 7},28, 40, 6, 0,  11, 45},
};

/* =========================================================
   CREATURE FUNCTIONS
   ========================================================= */

/* Stat calculation: base * 2 * level / 50 + 5 (HP gets +10+level) */
void creature_calc_stats(Creature *c) {
    const SpeciesData *sp = &species_db[c->species];
    uint16_t lv = c->level;

    c->max_hp = (uint16_t)((sp->base_hp * 2 * lv) / 50 + lv + 10);
    c->atk    = (uint8_t)((sp->base_atk * 2 * lv) / 50 + 5);
    c->def    = (uint8_t)((sp->base_def * 2 * lv) / 50 + 5);
    c->spatk  = (uint8_t)((sp->base_spatk * 2 * lv) / 50 + 5);
    c->spdef  = (uint8_t)((sp->base_spdef * 2 * lv) / 50 + 5);
    c->speed  = (uint8_t)((sp->base_speed * 2 * lv) / 50 + 5);
}

/* Initialize a new creature */
void creature_init(Creature *c, uint8_t species, uint8_t level) {
    uint8_t i;
    const SpeciesData *sp = &species_db[species];

    c->species = species;
    c->level = level;
    c->exp = 0;
    c->status = STATUS_NONE;
    c->status_turns = 0;

    /* Set moves based on level */
    for (i = 0; i < MAX_MOVES; i++) {
        c->moves[i] = 0xFF; /* empty */
        c->pp[i] = 0;
    }

    /* Learn all moves up to current level */
    {
        uint8_t slot = 0;
        for (i = 0; i < MOVES_PER_SPECIES && slot < MAX_MOVES; i++) {
            if (sp->learn_levels[i] <= level) {
                c->moves[slot] = sp->learn_moves[i];
                c->pp[slot] = move_db[sp->learn_moves[i]].pp_max;
                slot++;
                /* Keep only the last MAX_MOVES */
                if (i >= MAX_MOVES) {
                    uint8_t j;
                    for (j = 0; j < MAX_MOVES - 1; j++) {
                        c->moves[j] = c->moves[j + 1];
                        c->pp[j] = c->pp[j + 1];
                    }
                    slot = MAX_MOVES - 1;
                    c->moves[slot] = sp->learn_moves[i];
                    c->pp[slot] = move_db[sp->learn_moves[i]].pp_max;
                }
            }
        }
    }

    creature_calc_stats(c);
    c->hp = c->max_hp;
}

/* EXP needed for a given level: level^3 * 4 / 5 */
uint16_t creature_exp_for_level(uint8_t level) {
    uint16_t lv = level;
    return (uint16_t)((lv * lv * lv * 4) / 5);
}

/* Check if creature can level up. Returns 1 if leveled. */
uint8_t creature_check_levelup(Creature *c) {
    uint16_t needed;
    if (c->level >= MAX_LEVEL) return 0;
    needed = creature_exp_for_level(c->level + 1);
    if (c->exp >= needed) {
        c->level++;
        creature_calc_stats(c);
        c->hp = c->max_hp; /* full heal on level up */

        /* Check for new moves */
        {
            const SpeciesData *sp = &species_db[c->species];
            uint8_t i;
            for (i = 0; i < MOVES_PER_SPECIES; i++) {
                if (sp->learn_levels[i] == c->level) {
                    /* Try to add to empty slot */
                    uint8_t j;
                    for (j = 0; j < MAX_MOVES; j++) {
                        if (c->moves[j] == 0xFF) {
                            c->moves[j] = sp->learn_moves[i];
                            c->pp[j] = move_db[sp->learn_moves[i]].pp_max;
                            break;
                        }
                    }
                    /* If no empty slot, replace last */
                    if (j == MAX_MOVES) {
                        c->moves[MAX_MOVES - 1] = sp->learn_moves[i];
                        c->pp[MAX_MOVES - 1] = move_db[sp->learn_moves[i]].pp_max;
                    }
                    break;
                }
            }
        }
        return 1;
    }
    return 0;
}

/* Check and apply evolution */
void creature_check_evolution(Creature *c) {
    const SpeciesData *sp = &species_db[c->species];
    if (sp->evolve_into != 0xFF && c->level >= sp->evolve_level) {
        c->species = sp->evolve_into;
        creature_calc_stats(c);
        c->hp = c->max_hp;
    }
}

/* Full heal */
void creature_heal_full(Creature *c) {
    creature_calc_stats(c);
    c->hp = c->max_hp;
    c->status = STATUS_NONE;
    c->status_turns = 0;
    {
        uint8_t i;
        for (i = 0; i < MAX_MOVES; i++) {
            if (c->moves[i] != 0xFF) {
                c->pp[i] = move_db[c->moves[i]].pp_max;
            }
        }
    }
}

/* Get palette index for a species based on type */
uint8_t creature_get_palette(uint8_t species) {
    uint8_t t = species_db[species].type1;
    if (t == TYPE_NORMAL) return 0;
    return t; /* type index maps to palette index */
}

/* =========================================================
   DAMAGE CALCULATION (Showdown-inspired)
   damage = ((2*level/5 + 2) * power * A/D) / 50 + 2
   Then apply STAB, type effectiveness, random factor
   ========================================================= */

/* Stat stage multiplier (numerator), denominator is always stage_denom */
static const uint8_t stage_num[] = {2, 2, 2, 2, 2, 2, 2, 3, 4, 5, 6, 7, 8};
static const uint8_t stage_den[] = {8, 7, 6, 5, 4, 3, 2, 2, 2, 2, 2, 2, 2};

static uint16_t apply_stage(uint16_t stat, int16_t stage) {
    int16_t idx = stage + 6; /* -6..+6 maps to 0..12 */
    if (idx < 0) idx = 0;
    if (idx > 12) idx = 12;
    return (stat * stage_num[idx]) / stage_den[idx];
}

uint16_t calc_damage(BattleSide *attacker, BattleSide *defender, uint8_t move_idx) {
    const MoveData *mv = &move_db[move_idx];
    const SpeciesData *atk_sp = &species_db[attacker->mon->species];
    uint16_t atk_stat, def_stat;
    uint16_t damage;
    uint8_t eff1, eff2;
    uint16_t level = attacker->mon->level;

    if (mv->power == 0) return 0;

    /* Physical vs Special split */
    if (mv->category == CAT_PHYSICAL) {
        atk_stat = apply_stage(attacker->mon->atk, attacker->atk_stage);
        def_stat = apply_stage(defender->mon->def, defender->def_stage);
        /* Burn halves physical attack */
        if (attacker->mon->status == STATUS_BURN) {
            atk_stat /= 2;
        }
    } else {
        atk_stat = apply_stage(attacker->mon->spatk, attacker->spatk_stage);
        def_stat = apply_stage(defender->mon->spdef, defender->spdef_stage);
    }

    if (atk_stat == 0) atk_stat = 1;
    if (def_stat == 0) def_stat = 1;

    /* Base damage formula */
    damage = ((2 * level / 5 + 2) * mv->power * atk_stat / def_stat) / 50 + 2;

    /* STAB (Same Type Attack Bonus) - 1.5x */
    if (mv->type == atk_sp->type1 || mv->type == atk_sp->type2) {
        damage = damage * 3 / 2;
    }

    /* Type effectiveness vs both defender types */
    eff1 = type_chart[mv->type][species_db[defender->mon->species].type1];
    eff2 = type_chart[mv->type][species_db[defender->mon->species].type2];

    /* Apply: multiply by eff/10 for each */
    damage = damage * eff1 / 10;
    if (species_db[defender->mon->species].type1 != species_db[defender->mon->species].type2) {
        damage = damage * eff2 / 10;
    }

    /* Random factor: 85-100% */
    damage = damage * (uint16_t)rng_range(85, 100) / 100;

    /* Minimum 1 damage if not immune */
    if (damage == 0 && eff1 > 0 && eff2 > 0) damage = 1;

    return damage;
}

/* Type name lookup */
const char *type_name(uint8_t type) {
    switch (type) {
        case TYPE_NORMAL: return "Normal";
        case TYPE_FIRE:   return "Fire";
        case TYPE_WATER:  return "Water";
        case TYPE_LEAF:   return "Leaf";
        case TYPE_BOLT:   return "Bolt";
        case TYPE_STONE:  return "Stone";
        case TYPE_SHADOW: return "Shadow";
        case TYPE_LIGHT:  return "Light";
        default:          return "???";
    }
}
