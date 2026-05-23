#pragma once

#include <stdint.h>

void adc_init(void);
uint_fast16_t adc_single(uint_fast8_t channel);
uint_fast32_t adc_sum(uint_fast8_t channel, uint_fast16_t num);
uint_fast16_t adc_average(uint_fast8_t channel, uint_fast16_t num);

#define ADC_N 80
#define ADC_MEASURE(c) adc_sum(c, ADC_N)
#define V(x)  (int32_t)((x) * 4096.0 * ADC_N /    5.0)
#define mV(x) (int32_t)((x) * 4096.0 * ADC_N / 5000.0)
#define Vcc V(5)
#define Vfloat(x) ((x) * (float)(5.0 / 4096.0 / ADC_N))
