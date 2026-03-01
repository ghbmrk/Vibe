#include "rng.h"
#include <gb/gb.h>

static uint16_t rng_state;

void rng_init(void) {
    /* Seed from DIV register for some entropy */
    rng_state = (uint16_t)DIV_REG;
    rng_state |= ((uint16_t)DIV_REG << 8);
    if (rng_state == 0) rng_state = 0xACE1;
}

/* xorshift16 PRNG */
uint8_t rng_next(void) {
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return (uint8_t)(rng_state & 0xFF);
}

uint16_t rng_next16(void) {
    rng_next();
    return rng_state;
}

/* Returns value in [min, max] inclusive */
uint8_t rng_range(uint8_t min, uint8_t max) {
    uint8_t range;
    if (min >= max) return min;
    range = max - min + 1;
    return min + (rng_next() % range);
}
