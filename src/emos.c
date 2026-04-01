#include <stdlib.h>
#include "adc.h"
#include "debug.h"
#include "emos.h"
#include "globals.h"
#include "probe.h"
#include "timer.h"

static const unsigned int channels[3] = {1, 3, 7};

static bool emos_n(unsigned int p0, unsigned int p1, unsigned int p2)
{
    probe_configure(p0, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    //FUN_0800577c(p2, p1);
    result.bd = 2.0f /* TODO */ < 1.0f;

    probe_configure(p0, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p2, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    float u0 = adc_average(channels[p0], 100) * 5.0f / 4095.0f;
    float u1 = adc_average(channels[p1], 100) * 5.0f / 4095.0f;
    if ((u0 < 4.5f) || (u1 > 0.3f))
    {
        return false;
    }

    probe_configure(p0, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    u0 = adc_average(channels[p0], 100) * 5.0f / 4095.0f;
    u1 = adc_average(channels[p1], 100) * 5.0f / 4095.0f;
    if ((u0 > 0.3f) || (u1 < 4.5f))
    {
        return false;
    }

    probe_configure(p0, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p1, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p2, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    float u2 = adc_average(channels[p2], 100) * 5.0f / 4095.0f; /* TODO: store */
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    //measure_small_cap(p2, p0, p1, 1);
    result.component = COMPONENT_EMOS;
    result.probes[0] = p0;
    result.probes[1] = p1;
    result.probes[2] = p2;
    result.subtype = 1;
    return true;
}

static bool emos_p(unsigned int p0, unsigned int p1, unsigned int p2)
{
    probe_configure(p0, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    //FUN_0800577c(probe1,probe2);
    result.bd = 2.0f /* TODO */ < 1.0f;

    probe_configure(p0, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(p2, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    float u0 = 5.0f - adc_average(channels[p0], 100) * 5.0f / 4095.0f;
    float u1 = 5.0f - adc_average(channels[p1], 100) * 5.0f / 4095.0f;
    if ((u0 < 4.5f) || (u1 > 0.6f))
    {
        return false;
    }

    probe_configure(p0, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    tim6_msleep(10);
    u0 = 5.0f - adc_average(channels[p0], 100) * 5.0f / 4095.0f;
    u1 = 5.0f - adc_average(channels[p1], 100) * 5.0f / 4095.0f;
    if ((u0 > 0.6f) || (u1 < 4.5f))
    {
        return false;
    }

    probe_configure(p0, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(p1, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p2, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    float u2 = adc_average(channels[p2], 100) * 5.0f / 4095.0f; /* TODO: store */
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    //measure_small_cap(p2, p0, p1, 1);
    result.component = COMPONENT_EMOS;
    result.probes[0] = p0;
    result.probes[1] = p1;
    result.probes[2] = p2;
    result.subtype = 2;
    return true;
}

bool emos(void)
{
    static const unsigned int probes[6][3] =
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
        if (emos_n(probes[i][0], probes[i][1], probes[i][2]))
        {
            /* out of order on purpose as in original firmware */
            probe_configure(result.probes[0], PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
            probe_configure(result.probes[2], PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
            probe_configure(result.probes[1], PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
            tim6_msleep(100);
            int a2 = adc_average(channels[result.probes[2]], 1000);
            int a1 = adc_average(channels[result.probes[1]], 1000);
            /* should be (a1 - a2) / a2 * Rd ??? */
            result.resistance = abs(a1 - a2) * 30.0f / a2;
            debug_log("r = (%d - %d) * 30 / %d = %f\n", a1, a2, a2, result.resistance);
            if (result.resistance < 1.0f)
            {
                if (result.resistance > 0.01f)
                {
                    return true;
                }
                /* WTF? add random digit 0...9 / 1000 */
            }
            else
            {
                result.resistance /= 5.0f;
            }
            return true;
        }
        if (emos_p(probes[i][0], probes[i][1], probes[i][2]))
        {
            probe_configure(result.probes[0], PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
            probe_configure(result.probes[2], PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
            probe_configure(result.probes[1], PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
            tim6_msleep(100);
            int a1 = adc_average(channels[result.probes[1]], 1000);
            int a2 = adc_average(channels[result.probes[2]], 1000);
            result.resistance = abs(a2 - a1) * 680.0f / a1;
            debug_log("r = (%d - %d) * 680 / %d = %f\n", a2, a1, a1, result.resistance);
            if (result.resistance < 1.0f)
            {
                if (result.resistance > 0.01f)
                {
                    return true;
                }
                /* WTF? add random digit 0...9 / 1000 */
            }
            else
            {
                result.resistance /= 4.0f;
            }
            return true;
        }
    }

    return false;
}
