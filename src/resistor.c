#include <math.h>
#include "adc.h"
#include "component.h"
#include "debug.h"
#include "globals.h"
#include "probe.h"
#include "resistor.h"
#include "timer.h"

static void resistor_range_try(unsigned int p0, unsigned int p1)
{
    resistor_measure(p0, p1, 3);
    if (result.resistance > 15e3f) { return; }
    resistor_measure(p0, p1, 2);
    if (result.resistance > 0.8f) { return; }
    resistor_measure(p0, p1, 0);
}

bool resistor(void)
{
    static const unsigned int probes[3][2] = { {0, 1}, {0, 2}, {1, 2} };
    if ((result.component == COMPONENT_NONE) || (result.component == COMPONENT_CAP))
    {
        /* not a capacitor or below 700nF? */
        if ((result.component != COMPONENT_CAP) || (result.capacitance_pF < 700e3f))
        {
            float r_min = INFINITY; /* original firmware uses 1Gohm */
            int min_idx = 0;
            for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
            {
                probe_configure(0, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
                probe_configure(1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
                probe_configure(2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
                resistor_range_try(probes[i][0], probes[i][1]);
                debug_log("%u %u %f\n", probes[i][0], probes[i][1], result.resistance);
                if (!(r_min < result.resistance))
                {
                    r_min = result.resistance;
                    min_idx = i;
                }
            }
            if (0x0FA51F7B <= (uint32_t)result.capacitance_pF + 0xBB767FFF) // WTF?
            {
                float r = r_min * 1.3e8f / (1.3e8f - r_min);
                if (r < 30e6f)
                {
                    debug_log("r_min=%.1f idx=%u r=%.1f\n", r_min, min_idx, r);
                    result.resistance = r;
                    result.component = COMPONENT_RESISTOR;
                    result.probes[0] = probes[min_idx][0];
                    result.probes[2] = probes[min_idx][1];
                    return true;
                }
            } else { debug_log("cap=%f, reject resistance\n", result.capacitance_pF); }
        }
    }
    return false;
}

void resistor_measure(int a, int b, int param)
{
    static const unsigned int channels[3] = {1, 3, 7};
    float r;
    adc_sampletime = 14;
    if (param == 0)
    {
        /* very low resistance < 0.8ohm, connect directly */
        probe_configure(a, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
        probe_configure(b, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
        tim6_msleep(1);
        float aa = adc_average(channels[a], 1000);
        float ab = adc_average(channels[b], 1000);
        /* why add Rp??? */
        result.resistance = (ab - aa) * (calibration.rd + calibration.rp) / aa;
    }
    else
    {
        if (param != 1)
        {
            if (param == 2)
            {
                /* medium resistance < 15kohm, use 2x680R */
                probe_configure(a, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
                probe_configure(b, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
                tim6_msleep(100);
                float aa = adc_average(channels[a], 1000);
                float ab = adc_average(channels[b], 1000);
                result.resistance = (ab - aa) * (calibration.rd + 680.0f) / aa;
            }
            else
            {
                if (param != 3) { goto out; }
                probe_configure(a, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
                probe_configure(b, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
                tim6_msleep(100);
                float aa = adc_average(channels[a], 1000);
                float ab = adc_average(channels[b], 1000);
                result.resistance = (ab - aa) * (calibration.rd + 470e3f) / aa;
                debug_log("measure 3 probes:%u %u adc=%.0f %.0f R=%.0f\n", a, b, aa, ab, result.resistance);
            }
        }
        else
        {
            probe_configure(a, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
            probe_configure(b, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
            tim6_msleep(100);
            float ua = adc_average(channels[a], 1000) * (5.0f / 4095.0f);
            float ub = adc_average(channels[b], 1000) * (5.0f / 4095.0f);
            result.resistance = (ub - ua) * 680.0f / (5.0f - ub);
        }
    }
out:
    adc_sampletime = 5;
}
