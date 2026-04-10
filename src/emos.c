#include <stdlib.h>
#include "adc.h"
#include "debug.h"
#include "globals.h"
#include "helpers.h"
#include "probe.h"
#include "timer.h"

static const unsigned int channels[3] = {1, 3, 7};

static bool emos_n(unsigned int pg, unsigned int pd, unsigned int ps)
{
    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, pg, pd, ps);

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    /* measure forward voltage of body diode */
    diode_forward_reverse(ps, pd);
    result.bd = result.diode_vf < 1.0f;

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pd, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_usleep(5); /* not in original firmware, required for simulation */
    float ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    float ud = adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    if ((ug < 4.5f) || (ud > 0.3f))
    {
        debug_log("gate driven high: bad Ug=%.2fV or Ud=%.2fV\n", ug, ud);
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    tim6_usleep(5); /* not in original firmware, required for simulation */
    ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    ud = adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    if ((ug > 0.3f) || (ud < 4.5f))
    {
        debug_log("gate driven low: bad Ug=%.2fV or Ud=%.2fV\n", ug, ud);
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pd, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    tim6_usleep(5); /* not in original firmware, required for simulation */
    result.emos_uth = 5.0f - adc_average(channels[ps], 100) * (5.0f / 4095.0f);
    probe_configure(pd, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    cap_small(ps, pg, pd, true);
    result.component = COMPONENT_EMOS;
    result.probes[0] = pg;
    result.probes[1] = pd;
    result.probes[2] = ps;
    result.subtype = 1;
    return true;
}

static bool emos_p(unsigned int pg, unsigned int pd, unsigned int ps)
{
    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, pg, pd, ps);

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    /* measure forward voltage of body diode */
    diode_forward_reverse(pd, ps);
    result.bd = result.diode_vf < 1.0f;

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(pd, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_usleep(5); /* not in original firmware, required for simulation */
    float ug = 5.0f - adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    float ud = 5.0f - adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    if ((ug < 4.5f) || (ud > 0.6f))
    {
        debug_log("gate driven low: bad Ug=%.2fV or Ud=%.2fV\n", ug, ud);
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    tim6_msleep(10);
    ug = 5.0f - adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    ud = 5.0f - adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    if ((ug > 0.6f) || (ud < 4.5f))
    {
        debug_log("gate driven high: bad Ug=%.2fV or Ud=%.2fV\n", ug, ud);
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(pd, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    tim6_usleep(5); /* not in original firmware, required for simulation */
    result.emos_uth = adc_average(channels[ps], 100) * (5.0f / 4095.0f);
    probe_configure(pd, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    cap_small(ps, pg, pd, true);
    result.component = COMPONENT_EMOS;
    result.probes[0] = pg;
    result.probes[1] = pd;
    result.probes[2] = ps;
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
            result.resistance = divf(abs(a1 - a2) * 30.0f, a2);
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
            result.resistance = divf(abs(a2 - a1) * 680.0f, a1);
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
