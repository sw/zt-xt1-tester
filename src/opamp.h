#pragma once

#include <stdbool.h>

#ifdef __ARM_EABI__
    #include "n32g031_opamp.h"
    #define opamp_enable OPAMP_Enable
#else
    void opamp_enable(bool enable);
#endif
