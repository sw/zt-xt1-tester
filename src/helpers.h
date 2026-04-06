#pragma once

[[nodiscard]] float divf(float x, float y);

#ifdef __ARM_EABI__
    #define iwdg_reload() IWDG_ReloadKey()
#else
    #define iwdg_reload()
#endif
