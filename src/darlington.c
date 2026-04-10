#include "adc.h"
#include "darlington.h"
#include "globals.h"
#include "helpers.h"
#include "probe.h"
#include "timer.h"

static bool darlington_npn(unsigned int pb, unsigned int pc, unsigned int pe)
{
    static const unsigned int channels[3] = {1, 3, 7};

    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, pb, pc, pe);

    probe_configure(pb, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pc, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(pe, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
#ifndef __ARM_EABI__
    tim6_usleep(100); /* not in original firmware, required for simulation */
#endif
    float ub = adc_average(channels[pb], 100) * (5.0f / 4095.0f);
    float uc = adc_average(channels[pc], 100) * (5.0f / 4095.0f);
    float ue = adc_average(channels[pe], 100) * (5.0f / 4095.0f);
    debug_log("Ub=%.3fV Uc=%.3fV Ue=%.3fV\n", ub, uc, ue);
    float ib = (5.0f - ub) / (680.0f + calibration.rp);
    debug_log("Ib = (5V - %.3fV) / (680ohm + %.0fohm) = %.2fmA\n", ub, calibration.rp, ib * 1e3f);
    float ic = (5.0f - uc) / calibration.rp;
    debug_log("Ic = (5V - %.3fV) / %.0fohm = %.2fmA\n", uc, calibration.rp, ic * 1e3f);
    result.hfe = divf(ic, ib);
    debug_log("hFE = %.1f\n", result.hfe);
    result.bjt_ube = ub - ue;
    debug_log("Ube = %.3fV - %.3fV = %.3fV\n", ub, ue, result.bjt_ube);
    if ((result.bjt_ube < 1.0f) || (result.bjt_ube > 1.7f))
    {
        debug_log("bad Ube\n");
        return false;
    }
    if ((result.hfe < 10.0f) || (result.hfe > 600.0f))
    {
        debug_log("bad hFE\n");
        return false;
    }
    if (((uc - ue) < 0.2f) || ((uc - ue) > 1.0f))
    {
        debug_log("bad Uce\n");
        return false;
    }

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pc, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pe, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
#ifndef __ARM_EABI__
    tim6_usleep(100); /* not in original firmware, required for simulation */
#endif
    adc_average(channels[pb], 100);
    uc = adc_average(channels[pc], 100) * (5.0f / 4095.0f);
    adc_average(channels[pe], 100);
    /* shouldn't we use rd? */
    result.ic_mA = (5.0f - uc) / (680.0f + calibration.rp) * 1e3f;
    debug_log("Ic = (5V - %.3fV) / (680ohm + %.0fohm) = %.1fuA\n", uc, calibration.rp, result.ic_mA * 1e3f);
    if (result.ic_mA > 0.1f)
    {
        debug_log("bad Ic\n");
        return false;
    }
    result.subtype = 1;
    return true;
}

static bool darlington_pnp(unsigned int pb, unsigned int pc, unsigned int pe)
{
    static const unsigned int channels[3] = {1, 3, 7};

    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, pb, pc, pe);

    probe_configure(pb, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(pc, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(pe, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
#ifndef __ARM_EABI__
    tim6_usleep(100); /* not in original firmware, required for simulation */
#endif
    float ub = adc_average(channels[pb], 100) * (5.0f / 4095.0f);
    float uc = adc_average(channels[pc], 100) * (5.0f / 4095.0f);
    float ue = adc_average(channels[pe], 100) * (5.0f / 4095.0f);
    debug_log("Ub=%.3fV Uc=%.3fV Ue=%.3fV\n", ub, uc, ue);
    float ib = ub / (680.0f + calibration.rd);
    debug_log("Ib = %.3fV / (680ohm + %.0fohm) = %.2fuA\n", ub, calibration.rd, ib * 1e6f);
    float ic = uc / calibration.rd;
    debug_log("Ic = %.3fV / %.0fohm = %.2fmA\n", uc, calibration.rd, ic * 1e3f);
    result.hfe = divf(ic, ib);
    debug_log("hFE = %.1f\n", result.hfe);
    result.bjt_ube = ue - ub;
    debug_log("Ube = %.3fV - %.3fV = %.3fV\n", ue, ub, result.bjt_ube);
    if ((result.bjt_ube < 1.0f) || (result.bjt_ube > 1.7f))
    {
        debug_log("bad Ube\n");
        return false;
    }
    if ((result.hfe < 10.0f) || (result.hfe > 600.0f))
    {
        debug_log("bad hFE\n");
        return false;
    }
    if (((ue - uc) < 0.2f) || ((ue - uc) > 1.0f))
    {
        debug_log("bad Uce\n");
        return false;
    }

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pc, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(pe, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
#ifndef __ARM_EABI__
    tim6_usleep(100); /* not in original firmware, required for simulation */
#endif
    adc_average(channels[pb], 100);
    uc = adc_average(channels[pc], 100) * (5.0f / 4095.0f);
    adc_average(channels[pe], 100);
    result.ic_mA = uc / (680.0f + calibration.rd) * 1e3f;
    debug_log("Ic = %.3fV / (680ohm + %.0fohm) = %.1fuA\n", uc, calibration.rd, result.ic_mA * 1e3f);
    if (result.ic_mA > 0.1f)
    {
        debug_log("bad Ic\n");
        return false;
    }
    result.subtype = 2;
    return true;
}

bool darlington(void)
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
    int npn_idx = -1;
    float npn_max_hfe = 0;
    int pnp_idx = -1;
    float pnp_max_hfe = 0;

    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        if (darlington_npn(probes[i][0], probes[i][1], probes[i][2]))
        {
            if (npn_max_hfe < result.hfe)
            {
                npn_max_hfe = result.hfe;
                npn_idx = i;
            }
        }
    }
    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        if (darlington_pnp(probes[i][0], probes[i][1], probes[i][2]))
        {
            if (pnp_max_hfe < result.hfe)
            {
                pnp_max_hfe = result.hfe;
                pnp_idx = i;
            }
        }
    }

    if ((npn_idx < 0) && (pnp_idx < 0))
    {
        return false;
    }

    result.component = COMPONENT_DARLINGTON;
    if (result.subtype == 1)
    {
        assert(0 <= npn_idx);
        assert(npn_idx < sizeof(probes) / sizeof(probes[0]));
        darlington_npn(probes[npn_idx][0], probes[npn_idx][1], probes[npn_idx][2]);
        result.probes[0] = probes[npn_idx][0];
        result.probes[1] = probes[npn_idx][1];
        result.probes[2] = probes[npn_idx][2];
        debug_log("NPN Darlington found: B=%d C=%d E=%d\n",
               result.probes[0] + 1,
               result.probes[1] + 1,
               result.probes[2] + 1);
    }
    else
    {
        assert(result.subtype == 2);
        assert(0 <= pnp_idx);
        assert(pnp_idx < sizeof(probes) / sizeof(probes[0]));
        darlington_pnp(probes[pnp_idx][0], probes[pnp_idx][1], probes[pnp_idx][2]);
        result.probes[0] = probes[pnp_idx][0];
        result.probes[1] = probes[pnp_idx][1];
        result.probes[2] = probes[pnp_idx][2];
        debug_log("PNP Darlington found: B=%d C=%d E=%d\n",
               result.probes[0] + 1,
               result.probes[1] + 1,
               result.probes[2] + 1);
    }
    return true;
}
