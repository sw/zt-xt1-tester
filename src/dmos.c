#include "adc.h"
#include "component.h"
#include "globals.h"
#include "probe.h"
#include "timer.h"

static bool dmos_probe(unsigned int pg, unsigned int pd, unsigned int ps)
{
    static const unsigned int channels[3] = {1, 3, 7};

    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, pg, pd, ps);

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pd, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
#ifndef __ARM_EABI__
    tim6_usleep(500); /* not in original firmware, required for simulation */
#endif
    float ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    float ud = adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    float us = adc_average(channels[ps], 100) * (5.0f / 4095.0f);
    debug_log("Ug=%.3fV Ud=%.3fV Us=%.3fV\n", ug, ud, us);
    float id = (5.0f - ud) / (680.0f + calibration.rp);
    result.ic_mA = id * 1e3f;
    result.resistance = (ud - us) / id;
    if ((ug > 0.5f) || (ud > 0.5f))
    {
        debug_log("bad Ug or Ud when gate pulled low\n");
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pd, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
#ifndef __ARM_EABI__
    tim6_usleep(10); /* not in original firmware, required for simulation */
#endif
    ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    ud = adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    adc_average(channels[ps], 100); /* result thrown away */
    debug_log("Ug=%.3fV Ud=%.3fV\n", ug, ud);
    if ((ug < 4.5f) || (ud > 0.5f))
    {
        debug_log("bad Ug or Ud when gate pulled high\n");
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pd, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
#ifndef __ARM_EABI__
    tim6_usleep(200); /* not in original firmware, required for simulation */
#endif
    ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    ud = adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    us = adc_average(channels[ps], 100) * (5.0f / 4095.0f);
    debug_log("Ug=%.3fV Ud=%.3fV Us=%.3fV\n", ug, ud, us);
    id = ud / (680.0f + calibration.rd) * 1e3f;
    result.diode_vf = us - ud;
    if (ug > 0.7f)
    {
        debug_log("bad Ug\n");
        return false;
    }
    if ((result.diode_vf < 0.3f) || (result.diode_vf > 1.5f))
    {
        debug_log("bad Us-Ud\n");
        return false;
    }
    if (!(result.ic_mA * 0.95f > id))
    {
        debug_log("bad channel current\n");
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pd, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    adc_average(channels[pg], 100); /* result thrown away */
    ud = adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    us = adc_average(channels[ps], 100) * (5.0f / 4095.0f);
    if (!((us - ud) < result.diode_vf))
    {
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pd, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    tim6_msleep(10);
    ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    ud = adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    us = adc_average(channels[ps], 100) * (5.0f / 4095.0f);
    float is = us / (680.0f + calibration.rd) * 1e3f;
    if (0.5f < ug)
    {
        return false;
    }
    result.emos_uth = us;

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pd, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(1);
    ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    ud = adc_average(channels[pd], 100) * (5.0f / 4095.0f);
    us = adc_average(channels[ps], 100) * (5.0f / 4095.0f);
    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(pd, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(ps, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    is = us / calibration.rd;
    if (6.0f < result.ic_mA)
    {
        result.ic_mA = is * 1e3f;
        result.resistance = (ud - us) / is;
    }

    cap_small(ps, pg, pd, true);

    result.component = COMPONENT_DMOS;
    result.probes[0] = pg;
    result.probes[1] = pd;
    result.probes[2] = ps;
    result.channel = CHANNEL_N;

    return true;
}

bool dmos(void)
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

    result.component = COMPONENT_NONE;
    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        if (dmos_probe(probes[i][0], probes[i][1], probes[i][2]))
        {
            return true;
        }
    }
    return false;
}
