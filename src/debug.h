#pragma once

#if __ARM_EABI__
    #define assert(c) if (!(c)) __builtin_unreachable()
    #define static_assert _Static_assert
    #define debug_log(...)
#else
    #include <assert.h>
    #include <stdio.h>
    #define debug_log(...) printf(__VA_ARGS__)
#endif
