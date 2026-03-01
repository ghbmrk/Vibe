#ifndef RNG_H
#define RNG_H

#include <stdint.h>

void rng_init(void);
uint8_t rng_next(void);
uint8_t rng_range(uint8_t min, uint8_t max);
uint16_t rng_next16(void);

#endif
