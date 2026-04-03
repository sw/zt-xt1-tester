#include <math.h>
#if __ARM_EABI__
#include "n32g031_gpio.h"
#include "n32g031_tim.h"
#endif
#include "comp.h"
#include "globals.h"
#include "probe.h"
#include "timer.h"

float inductor_comp(unsigned int p0, unsigned int p1)
{
#if __ARM_EABI__
    static GPIO_Module *const direct_gpios[3] = { GPIOA, GPIOA, GPIOA };
    static const uint16_t direct_pins[3] = { GPIO_PIN_1, GPIO_PIN_3, GPIO_PIN_7 };
#endif

    result.resistance = fabsf(result.resistance);

    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    const uint_fast8_t vref = 6;
    comp_init(p0, vref);
    tim6_msleep(10);

    /* drive p1 high */
#if __ARM_EABI__
    direct_gpios[p1]->PBSC = direct_pins[p1];
#endif
    static const uint_fast32_t timeout = 48000000 / 5;
    uint32_t cnt = comp_wait(timeout);
    if (cnt >= timeout)
    {
        cnt = 0;
    }

    /* drive p1 low */
#if __ARM_EABI__
    direct_gpios[p1]->PBC = direct_pins[p1];
#endif

    float l1 = logf(1.0f - (result.resistance + 30.0f) / 15.0f * vref / 63.0f);
    float l2 = -(int)cnt / 48.0f * (result.resistance + 30.0f) / l1;
    if (l2 < 265.0f)
    {
        return 0.0000051962050f * powf(l2, 4.0f) - 0.0046979402f * powf(l2, 3.0f) + 1.5850439f * powf(l2, 2.0f) - 235.26521f * l2 + 12937.127f;
    }
    else
    {
        if (1500.0f < l2)
        {
            return l2;
        }
        return -0.000055182136f * powf(l2, 2.0f) + 1.3664441f * l2 - 260.50504f;
    }
}
