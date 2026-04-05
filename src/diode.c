#include <math.h>
#include "adc.h"
#include "cap.h"
#include "diode.h"
#include "globals.h"
#include "probe.h"
#include "resistor.h"
#include "timer.h"

bool diode(void)
{
    static const unsigned int probes[6][3] =
    {
        /* different order than original firmware to simplify tests. TODO: fix tests instead */
        {0, 1, 2},
        {0, 2, 1},
        {1, 0, 2},
        {1, 2, 0},
        {2, 0, 1},
        {2, 1, 0},
    };
    float ir[6];
    int num = 0;
    float max_vf = 0;
    int max_idx = 0;

    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        probe_configure(0, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
        probe_configure(1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
        probe_configure(2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);

        diode_forward_reverse(probes[i][0], probes[i][1]);
        result.diode_vf_a[i] = result.diode_vf;
        tim6_msleep(20);

        diode_forward_reverse(probes[i][0], probes[i][1]);
        if (0.1f < fabsf(result.diode_vf_a[i] - result.diode_vf))
        {
            return false; /* TODO: simulate this case */
        }
        result.diode_vf_a[i] = result.diode_vf;
        ir[i] = result.diode_ir_mA;

        if (result.diode_vf < 0.18f)
        {
            return false;
        }
        if (result.diode_vf < 3.5f)
        {
            num++;
            if (max_vf < result.diode_vf)
            {
                max_vf = result.diode_vf;
                max_idx = i;
            }
        }
    }

    if (num == 3)
    {
        /* delete a 2x diode */
        result.diode_vf_a[max_idx] = 5.0f;
    }
    else if (num == 1)
    {
        result.component = COMPONENT_DIODE;
        result.diode_vf = result.diode_vf_a[max_idx];
        result.diode_ir_mA = ir[max_idx];
        cap_small(probes[max_idx][0], probes[max_idx][1], probes[max_idx][2], true);
        if (999.0f < result.capacitance_pF)
        {
            result.capacitance_pF = 0.0f;
        }
        result.probes[0] = probes[max_idx][0];
        result.probes[1] = probes[max_idx][1];
        debug_log("diode Uf=%f Ir=%fmA C=%fpF probes:%u %u\n", result.diode_vf, result.diode_ir_mA, result.capacitance_pF, result.probes[0], result.probes[1]);
        return true;
    }
    else if (num != 2)
    {
        return false;
    }

    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        if (   (i != max_idx)
            && (result.diode_vf_a[i] < 4.5f)
            && (result.diode_vf_a[max_idx] - result.diode_vf_a[i] < 0.1f))
        {
            resistor_measure(probes[i][0], probes[i][1], 1);
            float r1 = result.resistance;
            resistor_measure(probes[i][0], probes[i][1], 2);
            float r2 = result.resistance;
            if (fabsf(r1 - r2) < r1 * 0.1f)
            {
                return false;
            }
        }
    }
    debug_log("2 diode\n");
    result.component = COMPONENT_2DIODE;
    return true;
}

void diode_forward_reverse(unsigned int pa, unsigned int pk)
{
    static const unsigned int channels[3] = {1, 3, 7};

    probe_configure(pa, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pk, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    float ua = adc_average(channels[pa], 1000) * (5.0f / 4095.0f);
    float uk = adc_average(channels[pk], 1000) * (5.0f / 4095.0f);
    result.diode_vf = ua - uk;

    probe_configure(pa, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(pk, PROBE_ANALOG, PROBE_DRV_HI, PROBE_DRV_HI);
    tim6_msleep(1);
    probe_configure(pk, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    uk = adc_average(channels[pk], 1000) * (5.0f / 4095.0f);
    result.diode_ir_mA = (5.0f - uk) / 470.0f;
}
