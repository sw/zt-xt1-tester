#include <stdbool.h>
#include <stdio.h>
#include "adc.h"
#include "debug.h"
#include "globals.h"
#include "helpers.h"
#include "probe.h"
#include "timer.h"

static bool bjt_npn(unsigned int pb, unsigned int pc, unsigned int pe)
{
    static const unsigned int channels[3] = {1, 3, 7};

    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, pb, pc, pe);

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pc, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pe, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(1);
    float ub = adc_average(channels[pb], 100) * (5.0f / 4095.0f);
    float uc = adc_average(channels[pc], 100) * (5.0f / 4095.0f);
    float ue = adc_average(channels[pe], 100) * (5.0f / 4095.0f);
    debug_log("U0=%.2fV U1=%.2fV U2=%.2fV\n", ub, uc, ue);
    float ib = (5.0f - ub) / (470e3f + calibration.rp);
    debug_log("Ib = (5V - %.2fV) / (470kohm + %.0fohm) = %.2fuA\n", ub, calibration.rp, ib * 1e6f);
    float ic = (5.0f - uc) / (680.0f + calibration.rp);
    debug_log("Ic = (5V - %.2fV) / (680ohm + %.0fohm) = %.2fmA\n", uc, calibration.rp, ic * 1e3f);
    result.hfe = divf(ic, ib);
    debug_log("hFE = %.1f\n", result.hfe);
    result.bjt_ube = ub - ue;
    debug_log("Ube = %.3fV - %.3fV = %.3fV\n", ub, ue, result.bjt_ube);
    if ((result.bjt_ube < 0.5f) || (result.bjt_ube > 0.9f))
    {
        debug_log("bad Ube\n");
        return false;
    }
    if ((result.hfe < 10.0f) || (result.hfe > 600.0f))
    {
        debug_log("bad hFE\n");
        return false;
    }
    if ((uc < 0.05f) || (uc > 4.95))
    {
        debug_log("bad Uc\n");
        return false;
    }

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pe, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(1);
    uc = adc_average(channels[pc], 100) * (5.0f / 4095.0f);
    result.ic_mA = (5.0f - uc) / (470e3f + calibration.rd) * 1e3f;
    debug_log("Ic = (5V - %fV) / (470kohm + %.0fohm) = %.1fuA\n", uc, calibration.rd, result.ic_mA * 1e3f);
    if ((result.ic_mA > 0.5f) || (uc < 4.5f))
    {
        debug_log("bad Ic or Uc\n");
        return false;
    }

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(pe, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_usleep(1); /* not in original firmware, required for simulation */
    ub = adc_average(channels[pb], 100) * (5.0f / 4095.0f);
    debug_log("Ub = %.2fV\n", ub);
    if (ub > 2.5f)
    {
        debug_log("bad Ub\n");
        return false;
    }

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pe, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_usleep(1); /* not in original firmware, required for simulation */
    uc = adc_average(channels[pc], 100) * (5.0f / 4095.0f);
    ue = adc_average(channels[pe], 100) * (5.0f / 4095.0f);
    result.diode_vf = ue - uc;
    debug_log("NPN found! Uf = %.2fV - %.2fV = %.2fV\n", ue, uc, result.diode_vf);

    result.subtype = 1;
    return true;
}

static bool bjt_pnp(unsigned int pb, unsigned int pc, unsigned int pe)
{
    static const unsigned int channels[3] = {1, 3, 7};

    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, pb, pc, pe);

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pc, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(pe, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_usleep(50); /* not in original firmware, required for simulation */
    float ub = adc_average(channels[pb], 100) * (5.0f / 4095.0f);
    float uc = adc_average(channels[pc], 100) * (5.0f / 4095.0f);
    float ue = adc_average(channels[pe], 100) * (5.0f / 4095.0f);
    debug_log("Ub=%.3fV Uc=%.3fV Ue=%.3fV\n", ub, uc, ue);
    float ib = ub / (470e3f + calibration.rd);
    debug_log("Ib = %.2fV / (470kohm + %.0fohm) = %.2fuA\n", ub, calibration.rd, ib * 1e6f);
    float ic = uc / (680.0f + calibration.rd);
    debug_log("Ic = %.2fV / (680ohm + %.0fohm) = %.2fmA\n", uc, calibration.rd, ic * 1e3f);
    result.hfe = divf(ic, ib);
    debug_log("hFE = %.1f\n", result.hfe);
    result.bjt_ube = ue - ub;
    debug_log("Ube = %.3fV - %.3fV = %.3fV\n", ue, ub, result.bjt_ube);
    if ((result.bjt_ube < 0.5f) || (result.bjt_ube > 0.9f))
    {
        debug_log("bad Ube\n");
        return false;
    }
    if ((result.hfe < 10.0f) || (result.hfe > 600.0f))
    {
        debug_log("bad hFE\n");
        return false;
    }
    if ((uc < 0.05f) || (uc > 4.95))
    {
        debug_log("bad Uc\n");
        return false;
    }

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pe, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_usleep(50); /* not in original firmware, required for simulation */
    uc = adc_average(channels[pc], 100) * (5.0f / 4095.0f);
    result.ic_mA = uc / (470e3f + calibration.rd) * 1e3f;
    debug_log("Ic = %fV / (470kohm + %.0fohm) = %.1fuA\n", uc, calibration.rd, result.ic_mA * 1e3f);
    if ((result.ic_mA > 0.5f) || (uc > 0.5f))
    {
        debug_log("bad Ic or Uc\n");
        return false;
    }

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pe, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_usleep(50); /* not in original firmware, required for simulation */
    ub = adc_average(channels[pb], 100) * (5.0f / 4095.0f);
    debug_log("Ub = %.2fV\n", ub);
    if (ub < 2.5f)
    {
        debug_log("bad Ub\n");
        return false;
    }

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pe, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_usleep(50); /* not in original firmware, required for simulation */
    uc = adc_average(channels[pc], 100) * (5.0f / 4095.0f);
    ue = adc_average(channels[pe], 100) * (5.0f / 4095.0f);
    result.diode_vf = uc - ue;
    debug_log("PNP found! Uf = %.2fV - %.2fV = %.2fV\n", uc, ue, result.diode_vf);
    result.subtype = 2;
    return true;
}

bool bjt(void)
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
    float npn_max = 0;
    int pnp_idx = -1;
    float pnp_max = 0;

    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        if (bjt_npn(probes[i][0], probes[i][1], probes[i][2]))
        {
            if (npn_max < result.hfe)
            {
                npn_max = result.hfe;
                npn_idx = i;
            }
        }
    }
    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        if (bjt_pnp(probes[i][0], probes[i][1], probes[i][2]))
        {
            if (pnp_max < result.hfe)
            {
                pnp_max = result.hfe;
                pnp_idx = i;
            }
        }
    }

    if ((npn_idx < 0) && (pnp_idx < 0))
    {
        return false;
    }

    if (result.subtype == 1)
    {
        assert(0 <= npn_idx);
        assert(npn_idx < sizeof(probes) / sizeof(probes[0]));
        if (!bjt_npn(probes[npn_idx][0], probes[npn_idx][1], probes[npn_idx][2]))
        {
            return false;
        }
        result.probes[0] = probes[npn_idx][0];
        result.probes[1] = probes[npn_idx][1];
        result.probes[2] = probes[npn_idx][2];
        debug_log("NPN BJT found: B=%d C=%d E=%d\n",
               result.probes[0] + 1,
               result.probes[1] + 1,
               result.probes[2] + 1);
    }
    else
    {
        assert(result.subtype == 2);
        assert(0 <= pnp_idx);
        assert(pnp_idx < sizeof(probes) / sizeof(probes[0]));
        if (!bjt_pnp(probes[pnp_idx][0], probes[pnp_idx][1], probes[pnp_idx][2]))
        {
            return false;
        }
        result.probes[0] = probes[pnp_idx][0];
        result.probes[1] = probes[pnp_idx][1];
        result.probes[2] = probes[pnp_idx][2];
        debug_log("PNP BJT found: B=%d C=%d E=%d\n",
               result.probes[0] + 1,
               result.probes[1] + 1,
               result.probes[2] + 1);
    }
    result.component = COMPONENT_BJT;
    return true;
}
