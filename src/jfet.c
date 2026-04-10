#include "adc.h"
#include "component.h"
#include "debug.h"
#include "globals.h"
#include "probe.h"
#include "timer.h"

static bool jfet_n(unsigned int pd, unsigned int ps, unsigned int pg)
{
    static const unsigned int channels[3] = {1, 3, 7};

    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, pd, ps, pg);

    /* Ugs = 0 -> should conduct */
    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pd, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    float ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    float ud = adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    adc_average(channels[ps], 100); /* result thrown away */
    debug_log("Ug=%.3fV Ud=%.3fV\n", ug, ud);
    float id = (5.0f - ud) / (680.0f + calibration.rp) * 1e3f;
    debug_log("Id = (5V - %.3fV) / (680ohm + %.0f ohm) = %.3fmA\n", ud, calibration.rp, id);
    if (ug > 0.25f)
    {
        debug_log("Bad Ug\n");
        return false;
    }
    if (id < 0.1f)
    {
        debug_log("Bad Id\n");
        return false;
    }
    result.ic_mA = id;

    /* positive Ugs -> check for diode voltage drop */
    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pd, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    ud = adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    adc_average(channels[ps], 100); /* result thrown away */
    debug_log("Ug=%.3fV Ud=%.3fV\n", ug, ud);
    id = (5.0f - ud) / (680.0f + calibration.rp) * 1e3f;
    debug_log("Id = (5V - %.3fV) / (680ohm + %.0f ohm) = %.3fmA\n", ud, calibration.rp, id);
    if ((ug < 0.3f) || (ug > 1.3f))
    {
        debug_log("Bad Ug\n");
        return false;
    }
    if (id < 0.1f)
    {
        debug_log("Bad Id\n");
        return false;
    }

    /* swap drain and source */
    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(ps, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pd, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    adc_average(channels[pd], 100); /* result thrown away */
    float us = adc_average(channels[ps], 100) * (5.0f / 4095.0f);
    debug_log("Ug=%.3fV Us=%.3fV\n", ug, us);
    float is = (5.0f - us) / (680.0f + calibration.rp) * 1e3f;
    debug_log("Is = (5V - %.3fV) / (680ohm + %.0f ohm) = %.3fmA\n", us, calibration.rp, is);
    if ((ug < 0.3f) || (ug > 1.3f))
    {
        debug_log("Bad Ug\n");
        return false;
    }
    if ((is < 0.1f) || (is > result.ic_mA * 2.0f))
    {
        debug_log("Bad Is\n");
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pd, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    float ugd = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    adc_average(channels[pd], 100); /* result thrown away */

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(pd, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    float ugs = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    adc_average(channels[ps], 100); /* result thrown away */

    debug_log("Ugd=%.3f Ugs=%.3f\n", ugd, ugs);
    if (ugs > ugd)
    {
        debug_log("Drain/source swapped\n");
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(pd, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(1);
    adc_average(channels[pg], 100); /* result thrown away */
    adc_average(channels[pd], 100); /* result thrown away */
    us = adc_average(channels[ps], 100) * (5.0f / 4095.0f);

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(pd, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);

    if (6.0f < result.ic_mA)
    {
        result.ic_mA = us / calibration.rd * 1e3f;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pd, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    tim6_msleep(10);
    adc_average(channels[pg], 100); /* result thrown away */
    adc_average(channels[pd], 100); /* result thrown away */
    result.jfet_ug = adc_average(channels[ps], 100) * (5.0f / 4095.0f);

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pd, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    tim6_msleep(10);
    adc_average(channels[pg], 100); /* result thrown away */
    adc_average(channels[pd], 100); /* result thrown away */
    us = adc_average(channels[ps], 100) * (5.0f / 4095.0f);
    if (result.jfet_ug < 0.3f)
    {
        result.jfet_ug = us;
    }

    debug_log("Found n-channel JFET!\n");
    result.component = COMPONENT_JFET;
    result.subtype = 1;
    result.probes[0] = pg;
    result.probes[1] = pd;
    result.probes[2] = ps;
    return true;
}

static bool jfet_p(unsigned int pd, unsigned int pg, unsigned int ps)
{
    static const unsigned int channels[3] = {1, 3, 7};

    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, pd, pg, ps);

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pd, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    float ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    float ud = adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    float us = adc_average(channels[ps], 100) * (5.0f / 4095.0f);
    debug_log("Ug=%.3fV Ud=%.3fV Us=%.3fV\n", ug, ud, us);
    float id = ud / (680.0f + calibration.rd) * 1e3f;
    debug_log("Id = %.3fV / (680ohm + %.0f ohm) = %.3fmA\n", ud, calibration.rd, id);
    if (us - ug > 0.25f)
    {
        debug_log("Bad Us-Ug\n");
        return false;
    }
    if (id < 0.1f)
    {
        debug_log("Bad Id\n");
        return false;
    }
    result.ic_mA = id;

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pd, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    ud = adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    us = adc_average(channels[ps], 100) * (5.0f / 4095.0f);
    debug_log("Ug=%.3fV Ud=%.3fV Us=%.3fV\n", ug, ud, us);
    id = ud / (680.0f + calibration.rd) * 1e3f;
    debug_log("Id = %.3fV / (680ohm + %.0f ohm) = %.3fmA\n", ud, calibration.rd, id);
    if (((us - ug) < 0.3f) || ((us - ug) > 1.3f))
    {
        debug_log("Bad Us-Ug\n");
        return false;
    }
    if (id < 0.1f)
    {
        debug_log("Bad Id\n");
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(ps, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(pd, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    adc_average(channels[pd], 100); /* result thrown away */
    us = adc_average(channels[ps], 100) * (5.0f / 4095.0f);
    debug_log("Ug=%.3fV Us=%.3fV\n", ug, us);
    float is = us / (680.0f + calibration.rd) * 1e3f;
    debug_log("Is = %.3fV / (680ohm + %.0f ohm) = %.3fmA\n", us, calibration.rd, is);
    if (((us - ug) < 0.1f) || ((us - ug) > 1.3f))
    {
        debug_log("Bad Us-Ug\n");
        return false;
    }
    if (is < 0.1f)
    {
        debug_log("Bad Is\n");
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(pd, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    float ugd = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    adc_average(channels[pd], 100); /* result thrown away */

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(pd, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    float ugs = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    adc_average(channels[pd], 100); /* result thrown away */

    debug_log("Ugd=%.3f Ugs=%.3f\n", ugd, ugs);
    if (ugs > ugd)
    {
        debug_log("Drain/source swapped\n");
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pd, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(1);
    adc_average(channels[pg], 100); /* result thrown away */
    ud = adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    adc_average(channels[ps], 100); /* result thrown away */

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(pd, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);

    if (6.0f < result.ic_mA)
    {
        result.ic_mA = ud / calibration.rd * 1e3f;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pd, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    tim6_msleep(10);
    adc_average(channels[pg], 100); /* result thrown away */
    adc_average(channels[pd], 100); /* result thrown away */
    result.jfet_ug = adc_average(channels[ps], 100) * (5.0f / 4095.0f);

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pd, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    tim6_msleep(10);
    adc_average(channels[pg], 100); /* result thrown away */
    adc_average(channels[pd], 100); /* result thrown away */
    us = adc_average(channels[ps], 100) * (5.0f / 4095.0f);
    if (result.jfet_ug < 0.3f)
    {
        result.jfet_ug = us;
    }

    debug_log("Found p-channel JFET!\n");
    result.component = COMPONENT_JFET;
    result.subtype = 2;
    result.probes[0] = pg;
    result.probes[1] = pd;
    result.probes[2] = ps;
    return true;
}

bool jfet(void)
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
        if (   jfet_n(probes[i][0], probes[i][1], probes[i][2])
            || jfet_p(probes[i][0], probes[i][1], probes[i][2]) )
        {
            return true;
        }
    }
    return false;
}
