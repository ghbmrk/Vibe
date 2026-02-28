/*  rng.c  –  XorShift16 pseudo-random number generator.
 *  Small, fast, and good enough for game-play randomness.
 */

#include "rng.h"

static uint16_t rng_state = 1;

void rng_seed(uint16_t seed) {
    rng_state = seed ? seed : 1u;
}

uint16_t rng_next(void) {
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return rng_state;
}

uint8_t rng_range(uint8_t min, uint8_t max) {
    if (min >= max) return min;
    return min + (uint8_t)(rng_next() % (uint16_t)(max - min + 1u));
}
