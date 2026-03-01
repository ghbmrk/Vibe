#ifndef SAVE_H
#define SAVE_H

#include "common.h"

#define SAVE_MAGIC 0xCE

void save_init(void);
void save_write(void);
uint8_t save_load(void);
void save_erase(void);

#endif
