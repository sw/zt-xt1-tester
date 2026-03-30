#include <stdbool.h>
#include <stdio.h>
#include "adc.h"
#include "probes.h"
#include "timer.h"

float calib_rp = 15;
float calib_rd = 15;
float result_hfe;
float result_ic;
float result_ube;
unsigned int result_subtype;
unsigned int result_probes[3];

static bool bjt_npn(unsigned int p0, unsigned int p1, unsigned int p2)
{
    static const unsigned int channels[3] = {1, 3, 7};

    printf("%s(%u, %u, %u)\n", __FUNCTION__, p0, p1, p2);

    probe_configure(p0, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p2, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    msleep(1);
    float u0 = adc_average(channels[p0], 100) * 5.0f / 4095.0f;
    float u1 = adc_average(channels[p1], 100) * 5.0f / 4095.0f;
    float u2 = adc_average(channels[p2], 100) * 5.0f / 4095.0f;
    printf("U0=%.2fV U1=%.2fV U2=%.2fV\n", u0, u1, u2);
    float ib = (5.0f - u0) / (470000.0f + calib_rp);
    printf("Ib = (5V - %.2fV) / (470kohm + %.0fohm) = %.2fuA\n", u0, calib_rp, ib * 1e6f);
    float ic = (5.0f - u1) / (680.0f + calib_rp);
    printf("Ic = (5V - %.2fV) / (680ohm + %.0fohm) = %.2fmA\n", u1, calib_rp, ic * 1000.0f);
    result_hfe = ic / ib;
    printf("hFE = %.1f\n", result_hfe);
    result_ube = u0 - u2;
    printf("Ube = %.3fV - %.3fV = %.3fV\n", u0, u2, result_ube);
    if ((result_ube > 0.9f) || (result_hfe > 600.0f) || (u1 > 4.95f))
    {
        return false;
    }

    probe_configure(p0, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(p2, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    msleep(1);
    u1 = adc_average(channels[p1], 100) * 5.0f / 4095.0f;
    result_ic = (5.0f - u1) / (470000.0f + calib_rp);
    printf("Ic = (5V - %fV) / (470kohm + %.0fohm) = %.1fuA\n", u1, calib_rp, result_ic * 1e6f);
    if ((result_ic > 0.0005f) || (u1 < 4.5f))
    {
        return false;
    }

    probe_configure(p0, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p2, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    u0 = adc_average(channels[p0], 100) * 5.0f / 4095.0f;
    printf("U0 = %.2fV\n", u0);
    if (u0 > 2.5f)
    {
        return false;
    }

    probe_configure(p0, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(p2, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    u1 = adc_average(channels[p1], 100) * 5.0f / 4095.0f;
    u2 = adc_average(channels[p2], 100) * 5.0f / 4095.0f;
    printf("U? = %.2fV - %.2fV = %.2fV\n", u2, u1, u2 - u1);
    result_subtype = 1;
    return true;
}

static bool bjt_pnp(unsigned int p0, unsigned int p1, unsigned int p2)
{
    static const unsigned int channels[3] = {1, 3, 7};

    printf("%s(%u, %u, %u)\n", __FUNCTION__, p0, p1, p2);

    probe_configure(p0, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(p2, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    float u0 = adc_average(channels[p0], 100) * 5.0f / 4095.0f;
    float u1 = adc_average(channels[p1], 100) * 5.0f / 4095.0f;
    float u2 = adc_average(channels[p2], 100) * 5.0f / 4095.0f;
    printf("U0=%.2fV U1=%.2fV U2=%.2fV\n", u0, u1, u2);
    float ib = u0 / (470000.0f + calib_rd);
    printf("Ib = %.2fV / (470kohm + %.0fohm) = %.2fuA\n", u0, calib_rd, ib * 1e6f);
    float ic = u1 / (680.0f + calib_rd);
    printf("Ic = %.2fV / (680ohm + %.0fohm) = %.2fmA\n", u1, calib_rd, ic * 1000.0f);
    result_hfe = ic / ib;
    printf("hFE = %.1f\n", result_hfe);
    result_ube = u2 - u0;
    printf("Ube = %.3fV - %.3fV = %.3fV\n", u2, u0, result_ube);
    if ((result_ube > 0.9f) || (result_hfe > 600.0f) || (u0 > 4.95))
    {
        return false;
    }

    probe_configure(p0, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(p2, PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
    u1 = adc_average(channels[p1], 100) * 5.0f / 4095.0f;
    result_ic = u1 / (470000.0f + calib_rd);
    printf("Ic = %fV / (470kohm + %.0fohm) = %.1fuA\n", u1, calib_rd, result_ic * 1e6f);
    if ((result_ic > 0.0005f) || (u1 > 0.5f))
    {
        return false;
    }

    probe_configure(p0, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(p2, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    u0 = adc_average(channels[p0], 100) * 5.0f / 4095.0f;
    printf("U0 = %.2fV\n", u0);
    if (u0 < 2.5f)
    {
        return false;
    }

    probe_configure(p0, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    probe_configure(p2, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    u1 = adc_average(channels[p1], 100) * 5.0f / 4095.0f;
    u2 = adc_average(channels[p2], 100) * 5.0f / 4095.0f;
    printf("U? = %.2fV - %.2fV = %.2fV\n", u1, u2, u1 - u2);
    result_subtype = 2;
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

    for (int i = 0; i < 6; i++)
    {
        if (bjt_npn(probes[i][0], probes[i][1], probes[i][2]))
        {
            if (npn_max < result_hfe)
            {
                npn_max = result_hfe;
                npn_idx = i;
            }
        }
    }
    for (int i = 0; i < 6; i++)
    {
        if (bjt_pnp(probes[i][0], probes[i][1], probes[i][2]))
        {
            if (pnp_max < result_hfe)
            {
                pnp_max = result_hfe;
                pnp_idx = i;
            }
        }
    }

    if ((npn_idx < 0) && (pnp_idx < 0))
    {
        return false;
    }

    if (result_subtype == 1)
    {
        if (!bjt_npn(probes[npn_idx][0], probes[npn_idx][1], probes[npn_idx][2]))
        {
            return false;
        }
        result_probes[0] = probes[npn_idx][0];
        result_probes[1] = probes[npn_idx][1];
        result_probes[2] = probes[npn_idx][2];
        printf("NPN BJT found: B=%d C=%d E=%d\n",
               result_probes[0] + 1,
               result_probes[1] + 1,
               result_probes[2] + 1);
    }
    else
    {
        if (!bjt_pnp(probes[pnp_idx][0], probes[pnp_idx][1], probes[pnp_idx][2]))
        {
            return false;
        }
        result_probes[0] = probes[pnp_idx][0];
        result_probes[1] = probes[pnp_idx][1];
        result_probes[2] = probes[pnp_idx][2];
        printf("PNP BJT found: B=%d C=%d E=%d\n",
               result_probes[0] + 1,
               result_probes[1] + 1,
               result_probes[2] + 1);
    }

    return true;
}
