/*  save.c  –  SRAM persistence via MBC5 + Battery.
 *
 *  The save data lives at 0xA000 in SRAM bank 0.
 *  A magic byte and a simple checksum guard against corruption.
 */

#include "save.h"
#include <gb/gb.h>
#include <string.h>

/* Pointer into SRAM.  On MBC5, SRAM is at 0xA000-0xBFFF. */
static volatile SaveData *sram = (volatile SaveData *)0xA000u;

/* Simple XOR checksum over the save payload (excluding the checksum byte). */
static uint8_t calc_checksum(const SaveData *d) {
    const uint8_t *p = (const uint8_t *)d;
    uint8_t sum = 0;
    uint16_t i;
    /* Checksum everything except the last byte (which IS the checksum) */
    for (i = 0; i < sizeof(SaveData) - 1u; i++) {
        sum ^= p[i];
    }
    return sum;
}

uint8_t save_exists(void) {
    uint8_t ok;
    ENABLE_RAM_MBC5;
    ok = (sram->magic == SAVE_MAGIC) &&
         (sram->checksum == calc_checksum((const SaveData *)sram));
    DISABLE_RAM_MBC5;
    return ok;
}

void save_game(void) {
    SaveData tmp;
    uint8_t i;

    tmp.magic       = SAVE_MAGIC;
    tmp.party_count = party_count;
    for (i = 0; i < MAX_PARTY; i++) {
        memcpy(&tmp.party[i], &party[i], sizeof(Creature));
    }
    tmp.current_map = current_map;
    tmp.player_x    = player_x;
    tmp.player_y    = player_y;
    tmp.player_dir  = player_dir;
    tmp.battles_won = battles_won;
    tmp.catches     = total_catches;
    tmp.checksum    = calc_checksum(&tmp);

    ENABLE_RAM_MBC5;
    memcpy((void *)sram, &tmp, sizeof(SaveData));
    DISABLE_RAM_MBC5;
}

uint8_t load_game(void) {
    SaveData tmp;
    uint8_t i;

    ENABLE_RAM_MBC5;
    memcpy(&tmp, (const void *)sram, sizeof(SaveData));
    DISABLE_RAM_MBC5;

    if (tmp.magic != SAVE_MAGIC) return 0;
    if (tmp.checksum != calc_checksum(&tmp)) return 0;

    party_count   = tmp.party_count;
    for (i = 0; i < MAX_PARTY; i++) {
        memcpy(&party[i], &tmp.party[i], sizeof(Creature));
    }
    current_map   = tmp.current_map;
    player_x      = tmp.player_x;
    player_y      = tmp.player_y;
    player_dir    = tmp.player_dir;
    battles_won   = tmp.battles_won;
    total_catches = tmp.catches;

    return 1;
}
