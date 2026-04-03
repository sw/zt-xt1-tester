#pragma once

#include <stdint.h>

void comp_init(uint_fast8_t probe, uint_fast8_t vref_sel);
uint_fast32_t comp_wait(uint_fast32_t timeout);
