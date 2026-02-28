/* rng.h - XorShift16 pseudo-random number generator */
#ifndef RNG_H
#define RNG_H

#include <stdint.h>

void     rng_seed(uint16_t seed);
uint16_t rng_next(void);
uint8_t  rng_range(uint8_t lo, uint8_t hi);   /* inclusive */
uint8_t  rng_chance(uint8_t pct);              /* returns 1 if roll < pct */

#endif
