#include "adc.h"
#include "component.h"
#include "debug.h"
#include "globals.h"
#include "helpers.h"
#include "probe.h"
#include "timer.h"

static void resistor_range_try(unsigned int p0, unsigned int p1)
{
    debug_log("%s(%u, %u)\n", __FUNCTION__, p0, p1);
    resistor_measure(p0, p1, 3);
    if (result.resistance > 15e3f) { return; }
    resistor_measure(p0, p1, 2);
    if (result.resistance > 0.8f) { return; }
    resistor_measure(p0, p1, 0);
}

bool resistor(void)
{
    static const unsigned int probes[][2] = { {0, 1}, {0, 2}, {1, 2} };

    if ((result.component != COMPONENT_NONE) && (result.component != COMPONENT_CAP))
    {
        return false;
    }
    if ((result.component == COMPONENT_CAP) && (result.capacitance_pF > 700e3f))
    {
        return false;
    }

    float r_min = 1.0e9f;
    int min_idx = 0;
    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        probe_configure(0, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
        probe_configure(1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
        probe_configure(2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
        resistor_range_try(probes[i][0], probes[i][1]);
        if (r_min > result.resistance)
        {
            r_min = result.resistance;
            min_idx = i;
        }
    }

    if ((result.capacitance_pF > 1100.0f) && (result.capacitance_pF < 3e12f))
    {
        return false;
    }

    float r = r_min * 1.3e8f / (1.3e8f - r_min);
    if (r > 30e6f)
    {
        return false;
    }

    debug_log("r_min=%.1f idx=%u r=%.1f\n", r_min, min_idx, r);
    result.resistance = r;
    result.component = COMPONENT_RESISTOR;
    result.probes[0] = probes[min_idx][0];
    result.probes[2] = probes[min_idx][1];
    return true;
}

void resistor_measure(int a, int b, int param)
{
    static const unsigned int channels[3] = {1, 3, 7};
    float aa, ab;

    switch (param)
    {
        case 0:
            /* very low resistance < 0.8ohm, connect directly */
            probe_configure(a, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
            probe_configure(b, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
            tim6_msleep(1);
            aa = adc_average(channels[a], 1000);
            ab = adc_average(channels[b], 1000);
            /* why add Rp??? */
            result.resistance = divf((ab - aa) * (self_adjust_vals.rd + self_adjust_vals.rp), aa);
            debug_log("measure 0 probes:%u %u adc=%.0f %.0f R=%.0f\n", a, b, aa, ab, result.resistance);
            break;

        case 1:
            /* only used for detecting dual diodes */
            probe_configure(a, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
            probe_configure(b, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
            tim6_msleep(100);
            float ua = adc_average(channels[a], 1000) * (5.0f / 4095.0f);
            float ub = adc_average(channels[b], 1000) * (5.0f / 4095.0f);
            /* here Rp is missing */
            result.resistance = divf((ub - ua) * 680.0f, 5.0f - ub);
            debug_log("measure 1 probes:%u %u U=%.3f %.3f R=%.0f\n", a, b, ua, ub, result.resistance);
            break;

        case 2:
            /* medium resistance < 15kohm, use 2x680R */
            probe_configure(a, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
            probe_configure(b, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
            tim6_msleep(100);
            aa = adc_average(channels[a], 1000);
            ab = adc_average(channels[b], 1000);
            result.resistance = divf((ab - aa) * (self_adjust_vals.rd + 680.0f), aa);
            debug_log("measure 2 probes:%u %u adc=%.0f %.0f R=%.0f\n", a, b, aa, ab, result.resistance);
            break;

        case 3:
            probe_configure(a, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
            probe_configure(b, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
            tim6_msleep(100);

            uint_fast16_t uia = adc_average(channels[a], 1000);
#ifndef __ARM_EABI__
            /* hack to avoid weird values after correction in main resistor function. TODO: investigate */
            if (uia < 15) { uia = 15; }
#endif
            aa = uia;

            ab = adc_average(channels[b], 1000);
            result.resistance = divf((ab - aa) * (self_adjust_vals.rd + 470e3f), aa);
            debug_log("measure 3 probes:%u %u adc=%.0f %.0f R=%.0f\n", a, b, aa, ab, result.resistance);
            break;
    }
}

bool resistor_tool(void)
{
    static const unsigned int probes[][2] = { {0, 1}, {0, 2}, {1, 2} };

    float r_min = 1e9f;
    result.component = COMPONENT_NONE;
    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        probe_configure(0, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
        probe_configure(1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
        probe_configure(2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
        resistor_range_try(probes[i][0], probes[i][1]);
        if (!(r_min < result.resistance))
        {
            r_min = result.resistance;
            result.probes[0] = probes[i][0];
            result.probes[2] = probes[i][1];
        }
    }
    float r = r_min * 1.3e8f / (1.3e8f - r_min);
    if (r > 30e6f)
    {
        return false;
    }
    result.resistance = r;
    result.component = COMPONENT_RESISTOR;
    return true;
}
