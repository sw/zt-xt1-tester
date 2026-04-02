#include <math.h>
#if __ARM_EABI__
#include "n32g031_gpio.h"
#include "n32g031_tim.h"
#endif
#include "cap.h"
#include "comp.h"
#include "globals.h"
#include "probe.h"
#include "timer.h"

bool cap_small(unsigned int p0, unsigned int p1, unsigned int p2_unused, bool subtract_probe)
{
#if __ARM_EABI__
    static GPIO_Module *const r470k_gpios[3] = { GPIOA, GPIOA, GPIOB };
    static const uint16_t r470k_pins[3] = { GPIO_PIN_2, GPIO_PIN_5, GPIO_PIN_1 };
#else
    /* TODO: simulation */
    return false;
#endif

    float probe_cap;
    if (p1 == 0)
    {
        if (p0 == 1)
        {
            probe_cap = calibration.probe21_cap;
        }
        else
        {
            probe_cap = calibration.probe31_cap;
        }
    }
    else if (p1 == 1)
    {
        if (p0 == 0)
        {
            probe_cap = calibration.probe12_cap;
        }
        else
        {
            probe_cap = calibration.probe32_cap;
        }
    }
    else if (p0 == 0)
    {
        probe_cap = calibration.probe13_cap;
    }
    else
    {
        probe_cap = calibration.probe23_cap;
    }

    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_DRV_LO);
    tim6_msleep(2);
#if __ARM_EABI__
    comp_init(p1, 51);
#endif
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    tim3_cnt_comp = 0;
    uint32_t cnt = 0;
    tim3_expiry = 0;

#if __ARM_EABI__
    TIM_SetCnt(TIM3, 0);
    r470k_gpios[p1]->PBSC = r470k_pins[p1];
#endif

    do
    {
        if (tim3_cnt_comp != 0)
        {
            cnt += tim3_cnt_comp;
            /*
                each timer tick equates ~27fF.
                original firmware actually calls double log(5.25)
                shouldn't we divide by 64 instead?
            */
            /*                              pF     clock     R      -ln(1-51/63) */
            result.capacitance_pF = cnt * (1e12f / 48e6f / 470e3f / 1.658228077f);
            if (subtract_probe)
            {
                result.capacitance_pF -= probe_cap;
            }
            return true;
        }
        cnt = tim3_expiry << 16;
    }
    while (cnt < (48000000 / 10));
    return false;   /* timeout after 100ms */
}
