#include "component.h"
#include <math.h>
#ifdef __ARM_EABI__
#include "n32g031_gpio.h"
#endif
#include "comp.h"
#include "globals.h"
#include "probe.h"
#include "timer.h"

float inductor_comp(unsigned int p0, unsigned int p1)
{
#ifdef __ARM_EABI__
    static GPIO_Module *const direct_gpios[3] = { GPIOA, GPIOA, GPIOA };
    static const uint16_t direct_pins[3] = { GPIO_PIN_1, GPIO_PIN_3, GPIO_PIN_7 };
#else
    static const unsigned int direct_gpios[3] = { 0, 1, 2 };
    static const unsigned int direct_pins[3] = { 0, 0, 0 };
#endif
    debug_log("%s(%u, %u)\n", __FUNCTION__, p0, p1);

    result.resistance = fabsf(result.resistance);

    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    const uint_fast8_t vref = 6;
    comp_prepare(p0, vref);
    tim6_msleep(10);

    /* drive p1 high */
    static const uint_fast32_t timeout = 48000000 / 5;
    uint32_t cnt = comp_start(direct_gpios[p1], direct_pins[p1], timeout);
    if (cnt >= timeout)
    {
        cnt = 0;
    }

    /* drive p1 low */
#ifdef __ARM_EABI__
    direct_gpios[p1]->PBC = direct_pins[p1];
#else
    probe_configure(p1, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
#endif

    /* should probably use rp/rd */
    float l1 = logf(1.0f - (result.resistance + 30.0f) * (vref / 64.0f / 15.0f));
    float l2 = -(int)cnt / 48.0f * (result.resistance + 30.0f) / l1;
    debug_log("cnt=%u l2=%.1f\n", cnt, l2);

#ifndef __ARM_EABI__
    /* hack for simulation: avoid misclassing resistors as inductors */
    if (l2 < 180.0f)
    {
        return 0.0f;
    }
#endif

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
        /* in the simulation, this makes it worse */
        return -0.000055182136f * powf(l2, 2.0f) + 1.3664441f * l2 - 260.50504f;
    }
}

bool inductor(void)
{
    if ((result.resistance < 200.0f) && (result.component == COMPONENT_RESISTOR))
    {
        result.inductance_uH = inductor_comp(result.probes[0], result.probes[2]);
        float q = result.inductance_uH / result.resistance;
        debug_log("L=%.1f Q=%.1f\n", result.inductance_uH, q);
        if ((2 < q) && (result.inductance_uH > 2.5f) && (result.inductance_uH < 1000000.0f))
        {
            result.component = COMPONENT_INDUCTOR;
            return true;
        }
    }
    return false;
}

bool inductor_tool(void)
{
    result.component = COMPONENT_NONE;

    if (resistor_tool() && (result.resistance < 200.0f) && (result.component == COMPONENT_RESISTOR))
    {
        result.inductance_uH = inductor_comp(result.probes[0], result.probes[2]);
        if ((result.inductance_uH > 2.5f) && (result.inductance_uH < 1000000.0f))
        {
            result.component = COMPONENT_INDUCTOR;
            return true;
        }
    }
    return false;
}
