#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    PROBE_ANALOG,
    PROBE_DRV_LO,
    PROBE_DRV_HI,
    PROBE_MODE_NB,
} probe_mode_t;

void probe_configure(uint_fast8_t probe, probe_mode_t direct, probe_mode_t r680, probe_mode_t r470k);
void probe_discharge(uint_fast8_t p0, uint_fast8_t p1);
bool probe_all_shorted(void);
void probe_calibrate_resistance(void);
bool probe_all_open(void);
void probe_calibrate_capacitance(void);
