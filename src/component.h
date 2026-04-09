#pragma once

#include <stdint.h>

typedef enum : uint8_t
{
    COMPONENT_NONE       =  0,
    COMPONENT_BJT        =  4,
    COMPONENT_DARLINGTON =  5,
    COMPONENT_EMOS       =  7,
    COMPONENT_DIODE      = 11,
    COMPONENT_2DIODE     = 12,
    COMPONENT_BATTERY    = 13,
    COMPONENT_CAP        = 14,
    COMPONENT_RESISTOR   = 15,
    COMPONENT_INDUCTOR   = 16,
} component_t;

void component_do_all(void);
