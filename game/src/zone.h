/* zone.h - Procedural zone generation */
#ifndef ZONE_H
#define ZONE_H

#include "common.h"

/* Generate a new zone. Seeds RNG from zone_num. */
void zone_generate(ZoneData *z, uint8_t zone_num);

/* Generate the gym interior map */
void zone_generate_gym(ZoneData *z);

/* Get the encounter rate for a zone (percent per tall grass step) */
uint8_t zone_encounter_rate(uint8_t zone_num);

/* Get the theme name for display */
const char *zone_theme_name(uint8_t theme);

#endif
