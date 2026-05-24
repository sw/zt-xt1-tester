#pragma once

#include <stdint.h>

void comp_init(void);
void comp_prepare(uint_fast8_t probe, uint_fast8_t vref_sel);
#ifdef __ARM_EABI__
#include "n32g031.h"
uint_fast32_t comp_start(GPIO_Module *gpio, uint_fast16_t pin, uint_fast32_t timeout);
#else
uint_fast32_t comp_start(unsigned int probe_pull, unsigned int pullup, uint_fast32_t timeout);
#endif
