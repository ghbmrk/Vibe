/* save.c - SRAM persistence (MBC5 + Battery) */
#include "save.h"
#include "character.h"
#include "creature.h"

#define SAVE_MAGIC_A  0x49   /* 'I' */
#define SAVE_MAGIC_B  0x48   /* 'H' */

/* SRAM layout:
   0x0000: magic byte A
   0x0001: magic byte B
   0x0002: checksum (XOR of all data bytes)
   0x0003: save data starts (Character + Creature + zone/position info)
*/

/* SRAM pointer */
#define SRAM_BASE ((uint8_t *)0xA000)

static uint8_t calc_checksum(const uint8_t *data, uint16_t len) {
    uint8_t chk = 0;
    uint16_t i;
    for (i = 0; i < len; i++) {
        chk ^= data[i];
    }
    return chk;
}

/* Save data structure (packed into bytes) */
typedef struct {
    /* Character */
    uint8_t  char_level;
    uint16_t char_xp;
    uint8_t  char_cha, char_wis, char_lck, char_con;
    uint8_t  char_gear[GEAR_SLOTS];
    uint16_t char_gold;
    uint8_t  char_num_skills;
    uint8_t  char_skills[MAX_SKILLS];

    /* Creature */
    uint8_t  crea_species;
    uint8_t  crea_level;
    uint16_t crea_xp;
    uint8_t  crea_gear[GEAR_SLOTS];
    uint8_t  crea_moves[MAX_MOVES];
    uint8_t  crea_num_moves;
    uint8_t  crea_num_skills;
    uint8_t  crea_skills[MAX_SKILLS];

    /* World */
    uint8_t  zone_num;
    uint8_t  px, py;
    uint8_t  in_gym;
    uint16_t battles_won;
    uint16_t creatures_caught;
} SaveData;

void save_game(void) {
    SaveData sd;
    uint8_t chk;
    uint8_t *src;
    uint16_t i;

    memset(&sd, 0, sizeof(SaveData));

    /* Pack character */
    sd.char_level = game.player.level;
    sd.char_xp    = game.player.xp;
    sd.char_cha   = game.player.cha;
    sd.char_wis   = game.player.wis;
    sd.char_lck   = game.player.lck;
    sd.char_con   = game.player.con;
    memcpy(sd.char_gear, game.player.gear, GEAR_SLOTS);
    sd.char_gold  = game.player.gold;
    sd.char_num_skills = game.player.num_skills;
    memcpy(sd.char_skills, game.player.skills, MAX_SKILLS);

    /* Pack creature */
    sd.crea_species   = game.creature.species;
    sd.crea_level     = game.creature.level;
    sd.crea_xp        = game.creature.xp;
    memcpy(sd.crea_gear, game.creature.gear, GEAR_SLOTS);
    memcpy(sd.crea_moves, game.creature.moves, MAX_MOVES);
    sd.crea_num_moves = game.creature.num_moves;
    sd.crea_num_skills = game.creature.num_skills;
    memcpy(sd.crea_skills, game.creature.skills, MAX_SKILLS);

    /* Pack world */
    sd.zone_num = game.zone.zone_num;
    sd.px = game.px;
    sd.py = game.py;
    sd.in_gym = game.in_gym;
    sd.battles_won = game.battles_won;
    sd.creatures_caught = game.creatures_caught;

    /* Calculate checksum */
    chk = calc_checksum((uint8_t *)&sd, sizeof(SaveData));

    /* Write to SRAM */
    ENABLE_RAM;
    SRAM_BASE[0] = SAVE_MAGIC_A;
    SRAM_BASE[1] = SAVE_MAGIC_B;
    SRAM_BASE[2] = chk;

    src = (uint8_t *)&sd;
    for (i = 0; i < sizeof(SaveData); i++) {
        SRAM_BASE[3 + i] = src[i];
    }
    DISABLE_RAM;
}

uint8_t load_game(void) {
    SaveData sd;
    uint8_t chk;
    uint8_t *dst;
    uint16_t i;

    ENABLE_RAM;

    /* Check magic bytes */
    if (SRAM_BASE[0] != SAVE_MAGIC_A || SRAM_BASE[1] != SAVE_MAGIC_B) {
        DISABLE_RAM;
        return 0;
    }

    chk = SRAM_BASE[2];

    /* Read save data */
    dst = (uint8_t *)&sd;
    for (i = 0; i < sizeof(SaveData); i++) {
        dst[i] = SRAM_BASE[3 + i];
    }
    DISABLE_RAM;

    /* Verify checksum */
    if (calc_checksum((uint8_t *)&sd, sizeof(SaveData)) != chk) {
        return 0;
    }

    /* Unpack character */
    character_init(&game.player);
    game.player.level = sd.char_level;
    game.player.xp    = sd.char_xp;
    game.player.xp_next = character_xp_for_level(sd.char_level + 1);
    game.player.cha   = sd.char_cha;
    game.player.wis   = sd.char_wis;
    game.player.lck   = sd.char_lck;
    game.player.con   = sd.char_con;
    memcpy(game.player.gear, sd.char_gear, GEAR_SLOTS);
    game.player.gold  = sd.char_gold;
    game.player.num_skills = sd.char_num_skills;
    memcpy(game.player.skills, sd.char_skills, MAX_SKILLS);

    /* Unpack creature */
    creature_create(&game.creature, sd.crea_species, sd.crea_level);
    game.creature.xp = sd.crea_xp;
    memcpy(game.creature.gear, sd.crea_gear, GEAR_SLOTS);
    memcpy(game.creature.moves, sd.crea_moves, MAX_MOVES);
    game.creature.num_moves = sd.crea_num_moves;
    game.creature.num_skills = sd.crea_num_skills;
    memcpy(game.creature.skills, sd.crea_skills, MAX_SKILLS);
    creature_calc_stats(&game.creature);
    creature_heal(&game.creature);

    /* Unpack world */
    game.zone.zone_num = sd.zone_num;
    game.px = sd.px;
    game.py = sd.py;
    game.in_gym = sd.in_gym;
    game.battles_won = sd.battles_won;
    game.creatures_caught = sd.creatures_caught;

    return 1;
}

void clear_save(void) {
    ENABLE_RAM;
    SRAM_BASE[0] = 0x00;
    SRAM_BASE[1] = 0x00;
    DISABLE_RAM;
}
