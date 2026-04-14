#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "debug.h"

typedef struct
{
    uint32_t magic0;
    float probe12_cap;
    float probe13_cap;
    float probe21_cap;
    float probe23_cap;
    float probe31_cap;
    float probe32_cap;
    float rp;
    float rd;
    uint32_t magic1;
} calibration_t;
static_assert(sizeof(calibration_t) == 40);

typedef enum : uint8_t
{
    CALIB_IDLE                 = 0,
    CALIB_PROBES_CHECK_SHORTED = 1,
    CALIB_PROBES_RESISTANCE    = 2,
    CALIB_PROBES_CHECK_OPEN    = 3,
    CALIB_PROBES_CAPACITANCE   = 4,
    CALIB_STORE                = 5,
    CALIB_TIMEOUT              = 6,
} calib_step_t;

bool calib_load(void);
void calib_default(void);
void calib_write(void);
