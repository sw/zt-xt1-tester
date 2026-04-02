#pragma once

#include <stdint.h>

typedef enum : uint8_t
{
    COMPONENT_NONE   =  0,
    COMPONENT_BJT    =  4,
    COMPONENT_EMOS   =  7,
    COMPONENT_DIODE  = 11,
    COMPONENT_2DIODE = 12,
} component_t;

void component_do_all(void);
