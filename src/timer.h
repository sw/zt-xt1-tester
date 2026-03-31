#pragma once

#include <stdint.h>

void tim6_init(void);
void tim6_msleep(uint_fast32_t ms);
void tim6_usleep(uint_fast16_t us);
