#pragma once

#include <stdint.h>

typedef enum
{
    PROBE_ANALOG,
    PROBE_DRV_LO,
    PROBE_DRV_HI,
    PROBE_MODE_NB,
} probe_mode_t;

void probe_configure(uint_fast8_t probe, probe_mode_t direct, probe_mode_t r680, probe_mode_t r470k);
