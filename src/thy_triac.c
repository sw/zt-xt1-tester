#include "adc.h"
#include "component.h"
#include "debug.h"
#include "globals.h"
#include "probe.h"
#include "timer.h"

static bool thy_triac_probe(unsigned int p0, unsigned int p1, unsigned int pg)
{
    static const unsigned int channels[3] = {1, 3, 7};

    debug_log("\n%s(%u, %u, %u)\n", __FUNCTION__, p0, p1, pg);

    result.probes[0] = pg;
    result.probes[1] = p1;
    result.probes[2] = p0;

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p0, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(1);
    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(1);
    float ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    float u1 = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
    adc_average(channels[p0], 100); /* result thrown away */
    debug_log("Ug=%.3fV U1=%.3fV\n", ug, u1);
    if ((ug > 0.1f) || (u1 < 4.9f))
    {
        debug_log("Bad Ug or U1\n");
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(1);
    ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    adc_average(channels[p1], 100); /* result thrown away */
    float u0 = adc_average(channels[p0], 100) * (5.0f / 4095.0f);
    debug_log("Ug=%.3fV U0=%.3fV\n", ug, u0);
    /* gate -> anode/MT1 diode drop? */
    if ((ug < 0.2f) || (ug > 2.5f))
    {
        debug_log("Bad U2\n");
        return false;
    }
    result.bjt_ube = ug - u0;

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_usleep(100);
    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
#ifndef __ARM_EABI__
    tim6_usleep(10); /* not in original firmware, required for simulation */
#endif
    adc_average(channels[pg], 100); /* result thrown away */
    u1 = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
    u0 = adc_average(channels[p0], 100) * (5.0f / 4095.0f);
    debug_log("^680ohm U1=%.3fV U0=%.3fV\n", u1, u0);
    result.diode_vf = u1 - u0;
    if ((u1 > 2.5f) || (result.diode_vf < 0.5f))
    {
        probe_configure(pg, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
        probe_configure(p1, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
        probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
        tim6_usleep(100);
        probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
#ifndef __ARM_EABI__
        tim6_usleep(10); /* not in original firmware, required for simulation */
#endif
        ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
        u1 = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
        u0 = adc_average(channels[p0], 100) * (5.0f / 4095.0f);
        debug_log("^direct U1=%.3fV U0=%.3fV\n", u1, u0);
        probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
        result.diode_vf = u1 - u0;
        if ((u1 > 3.0f) || (result.diode_vf < 0.5f))
        {
            return false;
        }
    }
    debug_log("thyristor/TRIAC found!\n");
    return true;
}

static bool thy_triac_distinguish(unsigned int p0, unsigned int p1, unsigned int pg)
{
    static const unsigned int channels[3] = {1, 3, 7};

    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, p0, p1, pg);

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p0, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(1);
    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(p0, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
#ifndef __ARM_EABI__
    tim6_usleep(10); /* not in original firmware, required for simulation */
#endif
    float ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    float u1 = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
    float u0 = adc_average(channels[p0], 100) * (5.0f / 4095.0f);
    if (u1 > 2.5f)
    {
        return true;    /* TRIAC */
    }

    probe_configure(pg, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p0, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_usleep(100);
    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
#ifndef __ARM_EABI__
    tim6_usleep(10); /* not in original firmware, required for simulation */
#endif
    ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    u1 = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
    u0 = adc_average(channels[p0], 100) * (5.0f / 4095.0f);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    if (((u0 - u1) < 0.5f) || ((u0 - u1) > 2.5f))
    {
        return false; /* thyristor */
    }
    return true; /* TRIAC */
}

bool thy_triac(void)
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
        if (thy_triac_probe(probes[i][0], probes[i][1], probes[i][2]))
        {
            if (!thy_triac_distinguish(probes[i][0], probes[i][1], probes[i][2]))
            {
                result.component = COMPONENT_THYRISTOR;
                debug_log("Found thyristor!\n");
            }
            else
            {
                result.component = COMPONENT_TRIAC;
                debug_log("Found TRIAC!\n");
            }
            return true;
        }
    }
    return false;
}
