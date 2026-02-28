/* creature.c - Creature species data and stat calculations */
#include "creature.h"
#include "rng.h"

/* ── Species base stats ────────────────────────────────────── */
/*                     HP  ATK DEF SPD  TYPE          move1        move2 */
const SpeciesData species_table[SP_COUNT] = {
    { 20,  8,  5,  6,  ELEM_FIRE,     MOVE_TACKLE, MOVE_EMBER    },  /* EMBERON  */
    { 22,  6,  7,  5,  ELEM_WATER,    MOVE_TACKLE, MOVE_SPLASH   },  /* TIDALIN  */
    { 25,  7,  8,  4,  ELEM_EARTH,    MOVE_TACKLE, MOVE_ROCKFALL },  /* TERRAVOLT */
    { 18,  7,  4,  9,  ELEM_ELECTRIC, MOVE_TACKLE, MOVE_SPARK    },  /* ZAPPIX   */
    { 19,  9,  5,  7,  ELEM_SHADOW,   MOVE_TACKLE, MOVE_HEX      },  /* SHADRIX  */
    { 21,  6,  6,  8,  ELEM_LIGHT,    MOVE_TACKLE, MOVE_FLASH    },  /* LUMINOS  */
};

/* ── Move data ─────────────────────────────────────────────── */
/*                        type           pow  acc  sp */
const MoveData move_table[MOVE_COUNT] = {
    { ELEM_NORMAL,    5,  95,  2 },  /* TACKLE   */
    { ELEM_FIRE,      6,  90,  3 },  /* EMBER    */
    { ELEM_WATER,     6,  90,  3 },  /* SPLASH   */
    { ELEM_EARTH,     6,  90,  3 },  /* ROCKFALL */
    { ELEM_ELECTRIC,  6,  90,  3 },  /* SPARK    */
    { ELEM_SHADOW,    6,  90,  3 },  /* HEX      */
    { ELEM_LIGHT,     6,  90,  3 },  /* FLASH    */
    { ELEM_FIRE,      9,  80,  5 },  /* BLAZE    */
    { ELEM_WATER,     9,  80,  5 },  /* TORRENT  */
    { ELEM_EARTH,     9,  80,  5 },  /* QUAKE    */
    { ELEM_ELECTRIC,  9,  80,  5 },  /* THUNDER  */
    { ELEM_SHADOW,    9,  80,  5 },  /* VOID     */
    { ELEM_LIGHT,     9,  80,  5 },  /* RADIANCE */
    { ELEM_FIRE,     13,  70,  8 },  /* INFERNO  */
    { ELEM_WATER,    13,  70,  8 },  /* TSUNAMI  */
    { ELEM_EARTH,    13,  70,  8 },  /* TREMOR   */
    { ELEM_ELECTRIC, 13,  70,  8 },  /* SURGE    */
    { ELEM_SHADOW,   13,  70,  8 },  /* ECLIPSE  */
    { ELEM_LIGHT,    13,  70,  8 },  /* NOVA     */
    { ELEM_NORMAL,    7,  85,  3 },  /* STRIKE   */
    { ELEM_NORMAL,   10,  75,  5 },  /* SLAM     */
    { ELEM_NORMAL,    4, 100,  1 },  /* RUSH     */
};

const char *const species_names[SP_COUNT] = {
    "EMBERON", "TIDALIN", "TERRAVOLT",
    "ZAPPIX",  "SHADRIX", "LUMINOS"
};

const char *const move_names[MOVE_COUNT] = {
    "TACKLE",  "EMBER",   "SPLASH",  "ROCKFALL",
    "SPARK",   "HEX",     "FLASH",   "BLAZE",
    "TORRENT", "QUAKE",   "THUNDER", "VOID",
    "RADIANCE","INFERNO", "TSUNAMI", "TREMOR",
    "SURGE",   "ECLIPSE", "NOVA",    "STRIKE",
    "SLAM",    "RUSH"
};

const char *const elem_names[ELEM_COUNT] = {
    "FIRE", "WATER", "EARTH", "ELEC",
    "SHADOW", "LIGHT", "NORMAL"
};

/* ── XP curve ──────────────────────────────────────────────── */
uint16_t creature_xp_for_level(uint8_t level) {
    return (uint16_t)level * (uint16_t)level * 10;
}

/* ── Create creature ───────────────────────────────────────── */
void creature_create(Creature *c, uint8_t species, uint8_t level) {
    const SpeciesData *base = &species_table[species];
    uint8_t i;

    memset(c, 0, sizeof(Creature));
    c->species = species;
    c->level = level;
    c->type = base->type;
    c->xp = 0;
    c->xp_next = creature_xp_for_level(level + 1);

    /* Set starting moves */
    c->moves[0] = base->start_move1;
    c->moves[1] = base->start_move2;
    c->num_moves = 2;

    /* Init gear to none */
    for (i = 0; i < GEAR_SLOTS; i++) {
        c->gear[i] = GEAR_NONE;
    }

    creature_calc_stats(c);
    creature_heal(c);
}

/* ── Calculate derived stats ───────────────────────────────── */
void creature_calc_stats(Creature *c) {
    const SpeciesData *base = &species_table[c->species];
    uint8_t i;
    uint16_t hp, atk, def, spd, sp;

    /* Base + level scaling (use uint16_t to prevent overflow) */
    hp  = (uint16_t)base->hp  + (uint16_t)c->level * 3;
    atk = (uint16_t)base->atk + (uint16_t)c->level * 2;
    def = (uint16_t)base->def + (uint16_t)c->level * 2;
    spd = (uint16_t)base->spd + (uint16_t)c->level;
    sp  = 10 + (uint16_t)c->level * 2;

    /* Apply skill bonuses */
    for (i = 0; i < c->num_skills; i++) {
        switch (c->skills[i]) {
            case MSKILL_ATK_UP:       atk += 2; break;
            case MSKILL_DEF_UP:       def += 2; break;
            case MSKILL_SPD_UP:       spd += 2; break;
            case MSKILL_HP_UP:        hp  += 5; break;
            case MSKILL_TOUGHNESS:    hp  += 10; break;
            case MSKILL_QUICK_STRIKE: spd += 3; break;
        }
    }

    /* Clamp to uint8_t range */
    c->max_hp = (hp  > 255) ? 255 : (uint8_t)hp;
    c->atk    = (atk > 255) ? 255 : (uint8_t)atk;
    c->def    = (def > 255) ? 255 : (uint8_t)def;
    c->spd    = (spd > 255) ? 255 : (uint8_t)spd;
    c->max_sp = (sp  > 255) ? 255 : (uint8_t)sp;
}

/* ── Heal to full ──────────────────────────────────────────── */
void creature_heal(Creature *c) {
    c->hp = c->max_hp;
    c->sp = c->max_sp;
}

/* ── Award XP ──────────────────────────────────────────────── */
uint8_t creature_award_xp(Creature *c, uint16_t amount) {
    c->xp += amount;
    if (c->xp >= c->xp_next && c->level < MAX_LEVEL) {
        c->level++;
        c->xp = 0;
        c->xp_next = creature_xp_for_level(c->level + 1);
        creature_calc_stats(c);
        creature_heal(c);
        return 1;
    }
    return 0;
}

/* ── Type effectiveness ────────────────────────────────────── */
uint8_t type_effectiveness(uint8_t atk_type, uint8_t def_type) {
    /* Normal is always neutral */
    if (atk_type == ELEM_NORMAL || def_type == ELEM_NORMAL)
        return 100;

    /* Fire > Earth > Electric > Water > Fire */
    if ((atk_type == ELEM_FIRE     && def_type == ELEM_EARTH) ||
        (atk_type == ELEM_EARTH    && def_type == ELEM_ELECTRIC) ||
        (atk_type == ELEM_ELECTRIC && def_type == ELEM_WATER) ||
        (atk_type == ELEM_WATER    && def_type == ELEM_FIRE))
        return 150;

    if ((atk_type == ELEM_EARTH    && def_type == ELEM_FIRE) ||
        (atk_type == ELEM_ELECTRIC && def_type == ELEM_EARTH) ||
        (atk_type == ELEM_WATER    && def_type == ELEM_ELECTRIC) ||
        (atk_type == ELEM_FIRE     && def_type == ELEM_WATER))
        return 67;

    /* Shadow <-> Light: both super effective */
    if ((atk_type == ELEM_SHADOW && def_type == ELEM_LIGHT) ||
        (atk_type == ELEM_LIGHT  && def_type == ELEM_SHADOW))
        return 150;

    /* Same type: not very effective */
    if (atk_type == def_type)
        return 75;

    return 100;
}

/* ── STAB ──────────────────────────────────────────────────── */
uint8_t type_stab(uint8_t creature_type, uint8_t move_type) {
    if (move_type == ELEM_NORMAL) return 100;
    return (creature_type == move_type) ? 125 : 100;
}
