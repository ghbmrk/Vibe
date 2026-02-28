/* rng.c - XorShift16 PRNG */
#include "rng.h"

static uint16_t state = 1;

void rng_seed(uint16_t seed) {
    state = seed ? seed : 1;
}

uint16_t rng_next(void) {
    state ^= state << 7;
    state ^= state >> 9;
    state ^= state << 8;
    return state;
}

uint8_t rng_range(uint8_t lo, uint8_t hi) {
    if (lo >= hi) return lo;
    return lo + (uint8_t)(rng_next() % (uint16_t)(hi - lo + 1));
}

uint8_t rng_chance(uint8_t pct) {
    return (rng_next() % 100) < pct;
}
