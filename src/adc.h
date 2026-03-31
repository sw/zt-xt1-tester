#pragma once

#include <stdint.h>

void adc_init(void);
uint_fast16_t adc_average(uint_fast8_t channel, uint_fast16_t num);
