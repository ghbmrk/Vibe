#ifndef RNG_H
#define RNG_H

#include <stdint.h>

/* Seed the global game PRNG */
void     rng_seed(uint16_t seed);

/* Get next 16-bit pseudo-random value */
uint16_t rng_next(void);

/* Uniform random in [min, max] (inclusive) */
uint8_t  rng_range(uint8_t min, uint8_t max);

#endif /* RNG_H */
