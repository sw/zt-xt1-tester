#include "adc.h"
#include "component.h"
#include "debug.h"
#include "globals.h"
#include "probe.h"
#include "timer.h"

static bool igbt_probe(unsigned int pg, unsigned int pc, unsigned int pe)
{
    static const unsigned int channels[3] = {1, 3, 7};

    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, pg, pc, pe);

    probe_configure(pg, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    diode_forward_reverse(pe, pc);
    /* expect a free-wheeling diode (FRD) */
    if ((result.diode_vf < 0.3f) || (result.diode_vf > 1.0f))
    {
        debug_log("Bad diode Vf\n");
        return false;
    }

    result.bd = 1;

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pe, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
#ifndef __ARM_EABI__
    tim6_usleep(500); /* not in original firmware, required for simulation */
#endif
    float ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    float uc = adc_average(channels[pc], 100) * (5.0f / 4095.0f);
    debug_log("Ug=%.3fV Uc=%.3fV\n", ug, uc);
    if ((ug < 4.9f) || (uc > 1.2f))
    {
        debug_log("Bad Ug or Uc when pulling gate high\n");
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pe, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    uc = adc_average(channels[pc], 100) * (5.0f / 4095.0f);
    debug_log("Ug=%.3fV Uc=%.3fV\n", ug, uc);
    if ((ug > 0.1f) || (uc < 4.5f))
    {
        debug_log("Bad Ug or Uc when pulling gate low\n");
        return false;
    }

    probe_configure(pg, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pc, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(pe, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
#ifndef __ARM_EABI__
    tim6_usleep(500); /* not in original firmware, required for simulation */
#endif
    ug = adc_average(channels[pg], 100) * (5.0f / 4095.0f);
    float ue = adc_average(channels[pe], 100) * (5.0f / 4095.0f);
    debug_log("Ug=%.3fV Ue=%.3fV\n", ug, ue);
    result.emos_uth = ug - ue;
    if ((ug < 4.9f) || (ue > 2.5f))
    {
        debug_log("Bad Ug or Ue\n");
        return false;
    }

    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    cap_small(pe, pg, pc, true);
    result.component = COMPONENT_IGBT;
    result.junction = JUNCTION_NPN; /* well, not really... */
    result.probes[0] = pg;
    result.probes[1] = pc;
    result.probes[2] = pe;

    debug_log("Found IGBT!\n");
    return true;
}

bool igbt(void)
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
        if (igbt_probe(probes[i][0], probes[i][1], probes[i][2]))
        {
            return true;
        }
    }
    return false;
}
