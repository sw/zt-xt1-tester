#include "adc.h"
#include "component.h"
#include "globals.h"
#include "probe.h"
#include "timer.h"

static bool ujt_probe(unsigned int pe, unsigned int p1, unsigned int p2)
{
    static const unsigned int channels[3] = {1, 3, 7};

    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, pe, p1, p2);

    probe_configure(pe, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p2, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    tim6_msleep(1);
    float ue = adc_average(channels[pe], 100) * (5.0f / 4095.0f);
    float u1 = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
    float u2 = adc_average(channels[p2], 100) * (5.0f / 4095.0f);
    debug_log("Ue=%.3fV U1=%.3fV U2=%.3fV\n", ue, u1, u2);
    if (ue > 0.1f)
    {
        debug_log("Bad Ue\n");
        return false;
    }
    if ((u1 < 4.9f) || (u2 < 4.9f))
    {
        debug_log("Bad U1 or U2\n");
        return false;
    }

    probe_configure(pe, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p2, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    tim6_msleep(1);
    u1 = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
    u2 = adc_average(channels[p2], 100) * (5.0f / 4095.0f);
    debug_log("U1=%.3fV U2=%.3fV\n", u1, u2);
    float rbb = (u2 - u1) / u1 * (680.0f + calibration.rd);
    if ((rbb < 200.0f) || (rbb > 100000.0f))
    {
        debug_log("Bad Rbb\n");
        return false;
    }

    probe_configure(pe, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p2, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
#ifndef __ARM_EABI__
    tim6_usleep(10); /* not in original firmware, required for simulation */
#endif
    ue = adc_average(channels[pe], 100) * (5.0f / 4095.0f);
    u1 = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
    u2 = adc_average(channels[p2], 100) * (5.0f / 4095.0f);
    debug_log("Ue=%.3fV U1=%.3fV U2=%.3fV\n", ue, u1, u2);
    float r2 = (u1 - u2) / u2 * (680.0f + calibration.rd);

    if (   (r2 < rbb)
        && (20.0f < r2)
        && (0.3f < (ue - u1))
        && ((ue - u1) < 0.9f)
        && (rbb - r2 > r2) )
    {
        probe_configure(pe, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
        probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
        probe_configure(p2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
#ifndef __ARM_EABI__
        tim6_usleep(100); /* not in original firmware, required for simulation */
#endif
        ue = adc_average(channels[pe], 100) * (5.0f / 4095.0f);
        u1 = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
        u2 = adc_average(channels[p2], 100) * (5.0f / 4095.0f);
        debug_log("Ue=%.3fV U1=%.3fV U2=%.3fV\n", ue, u1, u2);

        float r3 = (u2 - u1) / u1 * (680.0f + calibration.rd);
        if (   (r3 < rbb)
            && (20.0f < r3)
            && (0.3f < (ue - u2))
            && ((ue - u2) < 0.9f) )
        {
            debug_log("Found UJT!\n");
            result.bjt_ube = ue - u2;
            result.resistance = rbb;
            return true;
        }
    }

    return false;
}

bool ujt(void)
{
    static const unsigned int probes[][3] =
    {
        {0, 1, 2},
        {0, 2, 1},
        {1, 0, 2},
        {1, 2, 0},
        {2, 0, 1},
        {2, 1, 0},
    };

    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        if (ujt_probe(probes[i][0], probes[i][1], probes[i][2]))
        {
            result.component = COMPONENT_UJT;
            result.probes[0] = probes[i][0];
            result.probes[2] = probes[i][1];    // !!!
            result.probes[1] = probes[i][2];    // !!!
            return true;
        }
    }
    return false;
}
