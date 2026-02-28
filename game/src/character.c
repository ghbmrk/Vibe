/* character.c - Player character logic */
#include "character.h"

/* ── Gear bonus tables (defined in shop.c, externed here) ─── */
/* For simplicity, gear bonus values are stored in the GearDef.
   We'll just use flat bonuses from equipped gear IDs.
   Gear ID mapping: see shop.c gear_catalog.
   Gear IDs 0-5 = character gear, 6-11 = creature gear.
   Each gear gives +1 to +5 of its target stat per zone tier. */

/* Forward: gear stat bonus lookup (defined in shop.c) */
extern const GearDef gear_catalog[MAX_GEAR_ITEMS];

void character_init(Character *ch) {
    uint8_t i;
    memset(ch, 0, sizeof(Character));
    ch->level = 1;
    ch->xp = 0;
    ch->xp_next = character_xp_for_level(2);
    ch->cha = 3;
    ch->wis = 3;
    ch->lck = 3;
    ch->con = 3;
    ch->gold = 100;
    ch->num_skills = 0;
    for (i = 0; i < GEAR_SLOTS; i++) {
        ch->gear[i] = GEAR_NONE;
    }
}

uint16_t character_xp_for_level(uint8_t level) {
    return (uint16_t)level * (uint16_t)level * 8;
}

uint8_t character_award_xp(Character *ch, uint16_t amount) {
    /* Apply CON multiplier */
    uint16_t mult = character_xp_mult(ch);
    amount = (amount * mult) / 100;

    ch->xp += amount;
    if (ch->xp >= ch->xp_next && ch->level < MAX_LEVEL) {
        ch->level++;
        ch->xp = 0;
        ch->xp_next = character_xp_for_level(ch->level + 1);
        return 1;
    }
    return 0;
}

/* ── Stat accessors with gear bonuses ──────────────────────── */
static uint8_t gear_bonus_for(const Character *ch, uint8_t slot) {
    if (ch->gear[slot] == GEAR_NONE) return 0;
    if (ch->gear[slot] >= MAX_GEAR_ITEMS) return 0;
    return (uint8_t)gear_catalog[ch->gear[slot]].bonus;
}

uint8_t character_get_cha(const Character *ch) {
    uint8_t val = ch->cha + gear_bonus_for(ch, GEAR_WEAPON);
    uint8_t i;
    for (i = 0; i < ch->num_skills; i++) {
        if (ch->skills[i] == CSKILL_CHA_UP) val++;
    }
    return val;
}

uint8_t character_get_wis(const Character *ch) {
    uint8_t val = ch->wis + gear_bonus_for(ch, GEAR_ARMOR);
    uint8_t i;
    for (i = 0; i < ch->num_skills; i++) {
        if (ch->skills[i] == CSKILL_WIS_UP) val++;
    }
    return val;
}

uint8_t character_get_lck(const Character *ch) {
    uint8_t val = ch->lck + gear_bonus_for(ch, GEAR_ACCESSORY);
    uint8_t i;
    for (i = 0; i < ch->num_skills; i++) {
        if (ch->skills[i] == CSKILL_LCK_UP) val++;
    }
    return val;
}

uint8_t character_get_con(const Character *ch) {
    uint8_t val = ch->con;
    uint8_t i;
    for (i = 0; i < ch->num_skills; i++) {
        if (ch->skills[i] == CSKILL_CON_UP) val++;
    }
    return val;
}

uint8_t character_catch_bonus(const Character *ch) {
    uint8_t cha = character_get_cha(ch);
    /* Each CHA point = +3% catch rate */
    return cha * 3;
}

uint16_t character_gold_mult(const Character *ch) {
    uint8_t lck = character_get_lck(ch);
    uint16_t mult = 100 + (uint16_t)lck * 10;
    uint8_t i;
    for (i = 0; i < ch->num_skills; i++) {
        if (ch->skills[i] == CSKILL_WAR_TROPHY) mult += 20;
    }
    return mult;
}

uint16_t character_xp_mult(const Character *ch) {
    uint8_t con = character_get_con(ch);
    uint16_t mult = 100 + (uint16_t)con * 10;
    uint8_t i;
    for (i = 0; i < ch->num_skills; i++) {
        if (ch->skills[i] == CSKILL_EMPERORS_LT) mult += 15;
    }
    return mult;
}

uint16_t character_cost_mult(const Character *ch) {
    uint8_t wis = character_get_wis(ch);
    int16_t mult = 100 - (int16_t)wis * 5;
    if (mult < 30) mult = 30;  /* min 30% of original cost */
    return (uint16_t)mult;
}

uint8_t character_has_skill(const Character *ch, uint8_t skill_id) {
    uint8_t i;
    for (i = 0; i < ch->num_skills; i++) {
        if (ch->skills[i] == skill_id) return 1;
    }
    return 0;
}

void character_calc_stats(Character *ch) {
    /* Stats are computed on-the-fly via getters, nothing to cache */
    (void)ch;
}
