/* shop.h - Vendor / gear shop */
#ifndef SHOP_H
#define SHOP_H

#include "common.h"

/* Gear catalog (12 items: 6 char + 6 creature) */
extern const GearDef gear_catalog[MAX_GEAR_ITEMS];

/* Run the shop UI. Player can buy gear for character or creature. */
void shop_run(uint8_t zone_num);

/* Get the effective cost of gear item after WIS discount */
uint16_t shop_effective_cost(uint8_t gear_id);

#endif
