/* skilltree.c - Procedural skill generation */
#include "skilltree.h"
#include "creature.h"
#include "rng.h"

/* ── Character skill descriptions ──────────────────────────── */
static const char *const char_skill_descs[CSKILL_COUNT] = {
    "CHA +1",            /* CHA_UP */
    "WIS +1",            /* WIS_UP */
    "LCK +1",            /* LCK_UP */
    "CON +1",            /* CON_UP */
    "IRON HALO",         /* creature -10% dmg */
    "BATTLE CRY",        /* creature +15% ATK 3T */
    "TACTICAL MIND",     /* speed tie win */
    "VETERANS EYE",      /* +10% catch */
    "BLESSED ROUNDS",    /* ignore 10% DEF */
    "EMPEROR LIGHT",     /* +15% XP */
    "WAR TROPHY",        /* +20% gold */
    "ARTIFICER",         /* gear +50% */
};

static const char *const creature_skill_descs[MSKILL_COUNT] = {
    "ATK +2",            /* ATK_UP */
    "DEF +2",            /* DEF_UP */
    "SPD +2",            /* SPD_UP */
    "HP +5",             /* HP_UP */
    "THICK HIDE",        /* -2 flat dmg */
    "FURY",              /* +25% ATK <50% HP */
    "REGEN",             /* heal 5%/turn */
    "TYPE MASTERY",      /* STAB +25% */
    "EVASION",           /* 10% dodge */
    "TOUGHNESS",         /* +10 HP */
    "COUNTER",           /* reflect 20% */
    "QUICK STRIKE",      /* +3 SPD */
};

/* ── Helper: pick unique random options ────────────────────── */
static uint8_t already_picked(const SkillOption opts[], uint8_t count,
                              uint8_t id, uint8_t is_move) {
    uint8_t i;
    for (i = 0; i < count; i++) {
        if (opts[i].is_move == is_move && opts[i].id == id) return 1;
    }
    return 0;
}

/* ── Character skill generation ────────────────────────────── */
void skilltree_gen_char_options(SkillOption opts[3]) {
    uint8_t i, id;
    for (i = 0; i < 3; i++) {
        do {
            /* 50% stat boost, 50% passive */
            if (rng_chance(50)) {
                id = rng_range(0, 3); /* CHA/WIS/LCK/CON UP */
            } else {
                id = rng_range(4, CSKILL_COUNT - 1);
            }
        } while (already_picked(opts, i, id, 0));

        opts[i].id = id;
        opts[i].is_move = 0;
        opts[i].move_id = 0;
    }
}

/* ── Creature skill generation ─────────────────────────────── */
void skilltree_gen_creature_options(SkillOption opts[3], const Creature *c) {
    uint8_t i;
    uint8_t roll;

    for (i = 0; i < 3; i++) {
        roll = rng_range(0, 99);

        if (roll < 35) {
            /* 35% stat boost */
            uint8_t id;
            do {
                id = rng_range(0, 3); /* ATK/DEF/SPD/HP UP */
            } while (already_picked(opts, i, id, 0));
            opts[i].id = id;
            opts[i].is_move = 0;
            opts[i].move_id = 0;
        } else if (roll < 65 && c->num_moves < MAX_MOVES) {
            /* 30% new move (if room) */
            uint8_t move_id;
            uint8_t j, has;
            do {
                /* Prefer same-type moves, but allow others */
                if (rng_chance(60)) {
                    /* Same-type: base move + tier offset */
                    uint8_t tier = rng_range(0, 2); /* 0=basic,1=mid,2=strong */
                    if (c->type < ELEM_NORMAL) {
                        move_id = 1 + c->type + tier * 6;
                        if (move_id >= MOVE_STRIKE) move_id = MOVE_STRIKE;
                    } else {
                        move_id = MOVE_STRIKE + rng_range(0, 2);
                    }
                } else {
                    move_id = rng_range(0, MOVE_COUNT - 1);
                }
                /* Check not already known */
                has = 0;
                for (j = 0; j < c->num_moves; j++) {
                    if (c->moves[j] == move_id) { has = 1; break; }
                }
            } while (has || already_picked(opts, i, move_id, 1));

            opts[i].id = 0;
            opts[i].is_move = 1;
            opts[i].move_id = move_id;
        } else {
            /* 35% passive ability */
            uint8_t id;
            do {
                id = rng_range(4, MSKILL_COUNT - 1);
            } while (already_picked(opts, i, id, 0));
            opts[i].id = id;
            opts[i].is_move = 0;
            opts[i].move_id = 0;
        }
    }
}

/* ── Apply skills ──────────────────────────────────────────── */
void skilltree_apply_char_skill(Character *ch, const SkillOption *opt) {
    if (ch->num_skills < MAX_SKILLS) {
        ch->skills[ch->num_skills++] = opt->id;
    }
}

void skilltree_apply_creature_skill(Creature *c, const SkillOption *opt) {
    if (opt->is_move) {
        if (c->num_moves < MAX_MOVES) {
            c->moves[c->num_moves++] = opt->move_id;
        }
    } else {
        if (c->num_skills < MAX_SKILLS) {
            c->skills[c->num_skills++] = opt->id;
        }
        creature_calc_stats(c);
    }
}

/* ── Descriptions ──────────────────────────────────────────── */
const char *skilltree_char_desc(uint8_t skill_id) {
    if (skill_id < CSKILL_COUNT) return char_skill_descs[skill_id];
    return "???";
}

const char *skilltree_creature_desc(const SkillOption *opt) {
    if (opt->is_move) {
        if (opt->move_id < MOVE_COUNT) return move_names[opt->move_id];
        return "???";
    }
    if (opt->id < MSKILL_COUNT) return creature_skill_descs[opt->id];
    return "???";
}
