#include "save.h"
#include <gb/gb.h>
#include <string.h>

/* SRAM save area */

void save_init(void) {
    memset(&save, 0, sizeof(SaveData));
    save.magic = 0;
    save.party_count = 0;
    save.current_zone = 0;
    save.zones_cleared = 0;
    save.gold = 50; /* starting gold */
    save.badges = 0;
    save.player_x = 3;
    save.player_y = 9;
    save.player_dir = DIR_DOWN;

    /* Starting items */
    save.items[ITEM_POTION] = 3;
}

void save_write(void) {
    uint8_t *src = (uint8_t *)&save;
    uint8_t *dst;
    uint16_t i;

    save.magic = SAVE_MAGIC;

    ENABLE_RAM_MBC5;
    dst = (uint8_t *)0xA000;
    for (i = 0; i < sizeof(SaveData); i++) {
        dst[i] = src[i];
    }
    DISABLE_RAM_MBC5;
}

uint8_t save_load(void) {
    uint8_t *dst = (uint8_t *)&save;
    uint8_t *src;
    uint16_t i;

    ENABLE_RAM_MBC5;
    src = (uint8_t *)0xA000;

    /* Check magic byte */
    if (src[0] != SAVE_MAGIC) {
        DISABLE_RAM_MBC5;
        return 0; /* no valid save */
    }

    for (i = 0; i < sizeof(SaveData); i++) {
        dst[i] = src[i];
    }
    DISABLE_RAM_MBC5;
    return 1;
}

void save_erase(void) {
    uint8_t *dst;
    uint16_t i;

    ENABLE_RAM_MBC5;
    dst = (uint8_t *)0xA000;
    for (i = 0; i < sizeof(SaveData); i++) {
        dst[i] = 0;
    }
    DISABLE_RAM_MBC5;
}
