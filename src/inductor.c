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

    probe_discharge(p0, p1);
    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    const uint_fast8_t vref = 6;
    comp_prepare(p0, vref);
    tim6_msleep(1);

    /* drive p1 high */
    const uint_fast32_t timeout = 48000000 / 5;
    uint32_t cnt = comp_start(direct_gpios[p1], direct_pins[p1], timeout);
    probe_configure(p1, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    if (cnt >= timeout)
    {
        return 0.0f;
    }
#ifndef __ARM_EABI__
    /* hack for simulation: avoid misclassing resistors as inductors */
    if (cnt < 50)
    {
        return 0.0f;
    }
#endif

    /* L = -t_stop * R_total / ln(1 - (U_ref * R_total) / (5V * R_shunt)) */
    const float R_shunt = self_adjust_vals.rd;
    const float R_total = self_adjust_vals.rp + result.resistance + R_shunt;
    debug_log("cnt=%u\n", cnt);
    return cnt / -48.0f * R_total / logf(1.0f - vref / 64.0f * R_total / R_shunt);
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
