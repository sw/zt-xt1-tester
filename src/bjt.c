#include <math.h>
#include <stdbool.h>
#include "adc.h"
#include "debug.h"
#include "globals.h"
#include "probe.h"
#include "timer.h"

static bool bjt_npn(uint_fast8_t pb, uint_fast8_t pc, uint_fast8_t pe)
{
    static const uint_least8_t channels[3] = {1, 3, 7};

    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, pb, pc, pe);

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pc, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pe, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(5);
    int32_t ub = ADC_MEASURE(channels[pb]);
    int32_t uc = ADC_MEASURE(channels[pc]);
    int32_t ue = ADC_MEASURE(channels[pe]);
    debug_log("Ub=%.3fV Uc=%.3fV Ue=%.3fV\n", Vfloat(ub), Vfloat(uc), Vfloat(ue));
    int32_t ube = ub - ue;
    debug_log("Ube = %.3fV - %.3fV = %.3fV\n", Vfloat(ub), Vfloat(ue), Vfloat(ube));
    if ((ube < V(0.4)) || (ube > V(1.7)))
    {
        debug_log("bad Ube\n");
        return false;
    }
    if ((uc < V(0.05)) || (uc > V(4.95)))
    {
        debug_log("bad Uc\n");
        return false;
    }
    result.bjt_ube = Vfloat(ube);

    /* calculate hFE for common emitter circuit */
    result.hfe = 470e3f / (680.0f + self_adjust_vals.rp) * (Vcc - uc) / (Vcc - ub);
    debug_log("hFE (common emitter) = %.1f\n", result.hfe);

    /* measure hFE in common collector circuit, first with low base resistor */
    probe_configure(pb, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pc, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(pe, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    tim6_msleep(5);
    ub = Vcc - ADC_MEASURE(channels[pb]);
    ue = ADC_MEASURE(channels[pe]);
    float hfe = (float)(ue - ub) / ub;
    debug_log("hFE (common collector, low base resistor) = %.1f\n", hfe);
    /* for a Darlington or high-gain BJT, measure again with high base resistor */
    if (ub < mV(15))
    {
        probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
        tim6_msleep(5);
        ub = Vcc - ADC_MEASURE(channels[pb]);
        ue = ADC_MEASURE(channels[pe]);
        hfe = 470e3f / (680.0f + self_adjust_vals.rd) * ue / ub;
        debug_log("hFE (common collector, high base resistor) = %.1f\n", hfe);
    }
    result.hfe = fmaxf(result.hfe, hfe);

    if ((result.hfe < 10.0f) || (result.hfe > 75000.0f))
    {
        debug_log("bad hFE\n");
        return false;
    }

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pe, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(5);
    uc = ADC_MEASURE(channels[pc]);
    result.current_mA = (Vcc - uc) * (float)(1e3 / 470e3 * 5.0 / 4096.0 / ADC_N);
    debug_log("Ic = (5V - %.3fV) / 470kohm = %.1fuA\n", Vfloat(uc), result.current_mA * 1e3f);
    if ((result.current_mA > 0.5f) || (uc < V(4.5)))
    {
        debug_log("bad Ic or Uc\n");
        return false;
    }

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(pe, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(5);
    ub = ADC_MEASURE(channels[pb]);
    debug_log("Ub = %.3fV\n", Vfloat(ub));
    if (ub > V(2.5))
    {
        debug_log("bad Ub\n");
        return false;
    }

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pe, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(5);
    uc = ADC_MEASURE(channels[pc]);
    ue = ADC_MEASURE(channels[pe]);
    result.diode_vf = Vfloat(ue - uc);
    debug_log("NPN found! Uf = %.3fV - %.3fV = %.3fV\n", Vfloat(ue), Vfloat(uc), result.diode_vf);
    result.junction = JUNCTION_NPN;
    return true;
}

static bool bjt_pnp(uint_fast8_t pb, uint_fast8_t pc, uint_fast8_t pe)
{
    static const uint_least8_t channels[3] = {1, 3, 7};

    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, pb, pc, pe);

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pc, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(pe, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(5);
    int32_t ub = ADC_MEASURE(channels[pb]);
    int32_t uc = ADC_MEASURE(channels[pc]);
    int32_t ue = ADC_MEASURE(channels[pe]);
    debug_log("Ub=%.3fV Uc=%.3fV Ue=%.3fV\n", Vfloat(ub), Vfloat(uc), Vfloat(ue));
    int32_t ube = ue - ub;
    debug_log("Ube = %.3fV - %.3fV = %.3fV\n", Vfloat(ue), Vfloat(ub), Vfloat(ube));
    if ((ube < V(0.4)) || (ube > V(1.7)))
    {
        debug_log("bad Ube\n");
        return false;
    }
    if ((uc < V(0.05)) || (uc > V(4.95)))
    {
        debug_log("bad Uc\n");
        return false;
    }
    result.bjt_ube = Vfloat(ube);

    /* calculate hFE for common emitter circuit */
    result.hfe = 470e3f / (680.0f + self_adjust_vals.rd) * uc / ub;
    debug_log("hFE (common emitter) = %.1f\n", result.hfe);

    /* measure hFE in common collector circuit, first with low base resistor */
    probe_configure(pb, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(pc, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(pe, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    tim6_msleep(5);
    ub = ADC_MEASURE(channels[pb]);
    ue = Vcc - ADC_MEASURE(channels[pe]);
    float hfe = (float)(ue - ub) / ub;
    debug_log("hFE (common collector, low base resistor) = %.1f\n", hfe);
    /* for a Darlington or high-gain BJT, measure again with high base resistor */
    if (ub < mV(15))
    {
        probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
        tim6_msleep(5);
        ub = ADC_MEASURE(channels[pb]);
        ue = Vcc - ADC_MEASURE(channels[pe]);
        hfe = 470e3f / (680.0f + self_adjust_vals.rp) * ue / ub;
        debug_log("hFE (common collector, high base resistor) = %.1f\n", hfe);
    }
    result.hfe = fmaxf(result.hfe, hfe);

    if ((result.hfe < 10.0f) || (result.hfe > 75000.0f))
    {
        debug_log("bad hFE\n");
        return false;
    }

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pe, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(5);
    uc = ADC_MEASURE(channels[pc]);
    result.current_mA = uc * (float)(1e3 / 470e3 * 5.0 / 4096.0 / ADC_N);
    debug_log("Ic = %.3fV / 470kohm = %.1fuA\n", Vfloat(uc), result.current_mA * 1e3f);
    if ((result.current_mA > 0.5f) || (uc > V(0.5)))
    {
        debug_log("bad Ic or Uc\n");
        return false;
    }

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pe, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(5);
    ub = ADC_MEASURE(channels[pb]);
    debug_log("Ub = %.3fV\n", Vfloat(ub));
    if (ub < V(2.5))
    {
        debug_log("bad Ub\n");
        return false;
    }

    probe_configure(pb, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(pc, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(pe, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(5);
    uc = ADC_MEASURE(channels[pc]);
    ue = ADC_MEASURE(channels[pe]);
    result.diode_vf = Vfloat(uc - ue);
    debug_log("PNP found! Uf = %.3fV - %.3fV = %.3fV\n", Vfloat(uc), Vfloat(ue), result.diode_vf);
    result.junction = JUNCTION_PNP;
    return true;
}

bool bjt(void)
{
    static const uint_least8_t probes[][3] =
    {
        {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}
    };
    int npn_idx = -1;
    float npn_max_hfe = 0;
    int pnp_idx = -1;
    float pnp_max_hfe = 0;

    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        if (bjt_npn(probes[i][0], probes[i][1], probes[i][2]))
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
        if (bjt_pnp(probes[i][0], probes[i][1], probes[i][2]))
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

    if (result.junction == JUNCTION_NPN)
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
        assert(result.junction == JUNCTION_PNP);
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
    result.component = result.bjt_ube > 0.95f ? COMPONENT_DARLINGTON : COMPONENT_BJT;
    return true;
}
