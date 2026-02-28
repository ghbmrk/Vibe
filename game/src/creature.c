/*  creature.c  –  Species data, creature creation and stat growth.
 */

#include "creature.h"
#include "skilltree.h"
#include <string.h>

/* ---- Species table ---------------------------------------- */

const SpeciesData species_table[MAX_SPECIES] = {
    /*  name        type         HP ATK DEF SPD SPC catch */
    { "Emberon",  TYPE_FLAME,   45, 52, 43, 65, 50,  45 },
    { "Tidalin",  TYPE_AQUA,    50, 48, 55, 43, 55,  45 },
    { "Terravlt", TYPE_TERRA,   55, 55, 60, 35, 40,  45 },
    { "Zappix",   TYPE_VOLT,    40, 50, 40, 70, 55,  60 },
    { "Shadrix",  TYPE_SHADOW,  42, 58, 42, 62, 48,  60 },
    { "Luminos",  TYPE_AETHER,  48, 45, 50, 55, 60,  60 },
};

const char *type_names[NUM_TYPES] = {
    "FLAME", "AQUA", "TERRA", "VOLT", "SHADOW", "AETHER"
};

/* ---- Type effectiveness ----------------------------------- */

uint8_t type_effectiveness(uint8_t atk_type, uint8_t def_type) {
    /* Same type: resisted */
    if (atk_type == def_type) return 0;

    /* Circular: FLAME > TERRA > VOLT > AQUA > FLAME */
    if (atk_type == TYPE_FLAME  && def_type == TYPE_TERRA) return 2;
    if (atk_type == TYPE_TERRA  && def_type == TYPE_VOLT)  return 2;
    if (atk_type == TYPE_VOLT   && def_type == TYPE_AQUA)  return 2;
    if (atk_type == TYPE_AQUA   && def_type == TYPE_FLAME) return 2;

    /* Reverse of circular: not very effective */
    if (atk_type == TYPE_TERRA  && def_type == TYPE_FLAME) return 0;
    if (atk_type == TYPE_VOLT   && def_type == TYPE_TERRA) return 0;
    if (atk_type == TYPE_AQUA   && def_type == TYPE_VOLT)  return 0;
    if (atk_type == TYPE_FLAME  && def_type == TYPE_AQUA)  return 0;

    /* SHADOW <-> AETHER: mutually super effective */
    if (atk_type == TYPE_SHADOW && def_type == TYPE_AETHER) return 2;
    if (atk_type == TYPE_AETHER && def_type == TYPE_SHADOW) return 2;

    return 1; /* neutral */
}

/* ---- Experience curve ------------------------------------- */

uint16_t creature_exp_for_level(uint8_t level) {
    /* Simple curve: level^2 * 4 */
    return (uint16_t)level * (uint16_t)level * 4u;
}

/* ---- Stat calculation ------------------------------------- */

void creature_calc_stats(Creature *c) {
    const SpeciesData *sp = &species_table[c->species];
    uint16_t lv = c->level;
    uint8_t i;

    /* Base stats from species + level */
    c->max_hp = (uint16_t)((sp->base_hp  * 2u * lv) / 100u) + lv + 10u;
    c->atk    = (uint8_t) ((sp->base_atk * 2u * lv) / 100u) + 5u;
    c->def    = (uint8_t) ((sp->base_def * 2u * lv) / 100u) + 5u;
    c->spd    = (uint8_t) ((sp->base_spd * 2u * lv) / 100u) + 5u;
    c->spc    = (uint8_t) ((sp->base_spc * 2u * lv) / 100u) + 5u;
    c->sp_max = 10u + lv / 2u;

    /* Skill tree bonuses – each unlocked node modifies stats.
     * This is what makes two creatures of the same species different. */
    for (i = 1; i < c->tree.count; i++) {
        if (!NODE_UNLOCKED(c->tree.nodes[i])) continue;

        /* Passive keystones don't give flat stat bonuses –
         * their effects are handled in battle. */
        if (NODE_IS_PASSIVE(c->tree.nodes[i])) {
            /* GLASS_CANNON is applied after the loop */
            continue;
        }

        switch (c->tree.nodes[i].category) {
            case SKILL_ATTACK:  c->atk += 2; break;
            case SKILL_DEFEND:  c->def += 2; c->max_hp += 3; break;
            case SKILL_SUPPORT: c->spc += 2; c->max_hp += 2; break;
            case SKILL_SPECIAL: c->atk += 1; c->spc += 1; c->spd += 1; break;
        }
    }

    /* GLASS_CANNON keystone: +50% ATK, -30% DEF (permanent trade-off) */
    for (i = 1; i < c->tree.count; i++) {
        if (NODE_UNLOCKED(c->tree.nodes[i]) &&
            NODE_NTYPE(c->tree.nodes[i]) == NTYPE_GLASS) {
            c->atk = (uint8_t)((uint16_t)c->atk * 3u / 2u);
            c->def = (uint8_t)((uint16_t)c->def * 7u / 10u);
            if (c->def < 1) c->def = 1;
            break;
        }
    }
}

/* ---- Creature initialisation ------------------------------ */

void creature_init(Creature *c, uint8_t species, uint8_t level, uint16_t seed) {
    uint8_t i;
    const char *src;

    memset(c, 0, sizeof(Creature));
    c->species   = species;
    c->type      = species_table[species].type;
    c->level     = level;
    c->tree_seed = seed;

    /* Copy species name as default nickname */
    src = species_table[species].name;
    for (i = 0; i < NAME_LEN - 1 && src[i] != '\0'; i++) {
        c->name[i] = src[i];
    }
    c->name[i] = '\0';

    /* Derive stats */
    creature_calc_stats(c);
    c->hp       = c->max_hp;
    c->sp       = c->sp_max;
    c->exp      = 0;
    c->exp_next = creature_exp_for_level(level + 1);
    c->skill_pts = 1;  /* start with 1 point to spend */

    /* Build the procedural skill tree */
    skilltree_generate(&c->tree, c->type, seed);
    /* Root node always starts unlocked */
    NODE_UNLOCK(c->tree.nodes[0]);
}

/* ---- Level up --------------------------------------------- */

uint8_t creature_level_up(Creature *c) {
    uint16_t old_max;

    if (c->level >= MAX_LEVEL) return c->level;

    c->level++;
    c->exp     -= c->exp_next;
    c->exp_next = creature_exp_for_level(c->level + 1);

    old_max = c->max_hp;
    creature_calc_stats(c);

    /* Heal by the amount max HP increased */
    c->hp += (c->max_hp - old_max);
    if (c->hp > c->max_hp) c->hp = c->max_hp;

    /* Grant a skill point every 2 levels */
    if ((c->level & 1u) == 0) {
        c->skill_pts++;
    }
    return c->level;
}

void creature_gain_exp(Creature *c, uint16_t amount) {
    c->exp += amount;
    while (c->exp >= c->exp_next && c->level < MAX_LEVEL) {
        creature_level_up(c);
    }
}
