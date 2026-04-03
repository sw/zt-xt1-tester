#include <math.h>
#if __ARM_EABI__
#include "n32g031_gpio.h"
#include "n32g031_tim.h"
#endif
#include "adc.h"
#include "cap.h"
#include "comp.h"
#include "globals.h"
#include "probe.h"
#include "timer.h"

static void cap_bat_find(void)
{
    static const unsigned int channels[3] = {1, 3, 7};
    static const unsigned int probes[6][3] =
    {
        {0, 1, 2},
        {0, 2, 1},
        {1, 0, 2},
        {1, 2, 0},
        {2, 0, 1},
        {2, 1, 0},
    };
    result.component = COMPONENT_NONE;
    int i;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        probe_configure(probes[i][0], PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
        probe_configure(probes[i][1], PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
        probe_configure(probes[i][2], PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
        tim6_msleep(10);
        float u = adc_average(channels[probes[i][1]], 100) * (5.0f / 4095.0f);
        if (u < 1.0f)
        {
            probe_configure(probes[i][1], PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
            tim6_msleep(10);
            u = adc_average(channels[probes[i][1]], 100) * (5.0f / 4095.0f);
            if (u < 2.0f)
            {
                tim6_msleep(1000);
                float u2 = adc_average(channels[probes[i][1]], 100) * (5.0f / 4095.0f);
                if (!(u + 0.05f > u2))
                {
                    goto cap;
                }
            }
        }
        else
        {
            probe_configure(probes[i][1], PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
            tim6_msleep(10);
            u = adc_average(channels[probes[i][1]], 100) * (5.0f / 4095.0f);
            if (0.5f < u)
            {
                tim6_msleep(1000);
                float u2 = adc_average(channels[probes[i][1]], 100) * (5.0f / 4095.0f);
                if (u2 + 0.05f > u)
                {
                    debug_log("%s found battery\n", __FUNCTION__);
                    result.component = COMPONENT_BATTERY;
                }
                else
                {
    cap:
                    debug_log("%s found capacitor\n", __FUNCTION__);
                    result.component = COMPONENT_CAP;
                }
                break;
            }
        }
    }
    result.probes[0] = probes[i][0];
    result.probes[2] = probes[i][1]; // !!!
    result.probes[1] = probes[i][2]; // !!!
    probe_configure(0, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
}

bool cap_small(unsigned int p0, unsigned int p1, unsigned int p2_unused, bool subtract_probe)
{
#if __ARM_EABI__
    static GPIO_Module *const r470k_gpios[3] = { GPIOA, GPIOA, GPIOB };
    static const uint16_t r470k_pins[3] = { GPIO_PIN_2, GPIO_PIN_5, GPIO_PIN_1 };
#endif
    debug_log("%s(%u, %u)\n", __FUNCTION__, p0, p1);

    float probe_cap;
    if (p1 == 0)
    {
        if (p0 == 1)
        {
            probe_cap = calibration.probe21_cap;
        }
        else
        {
            probe_cap = calibration.probe31_cap;
        }
    }
    else if (p1 == 1)
    {
        if (p0 == 0)
        {
            probe_cap = calibration.probe12_cap;
        }
        else
        {
            probe_cap = calibration.probe32_cap;
        }
    }
    else if (p0 == 0)
    {
        probe_cap = calibration.probe13_cap;
    }
    else
    {
        probe_cap = calibration.probe23_cap;
    }

    /* p2 is not initialised in the original firmware, how does this even work??? */
    probe_configure(p2_unused, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);

    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_DRV_LO);
    tim6_msleep(2);
    const uint_fast8_t vref = 51;
    comp_init(p1, vref);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
#if __ARM_EABI__
    r470k_gpios[p1]->PBSC = r470k_pins[p1];
#else
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
#endif
    static const uint_fast32_t timeout = 48000000 / 10;
    uint32_t cnt = comp_wait(timeout);
    if (cnt >= timeout)
    {
        return false;
    }

    /*
        each timer tick equates ~27fF.
        original firmware actually calls double log(5.25)
        shouldn't we divide by 64 instead?
    */
    result.capacitance_pF = cnt * (1e12f / 48e6f / 470e3f / logf(63.0f / (63.0f - vref)));
    if (subtract_probe)
    {
        result.capacitance_pF -= probe_cap;
    }
    debug_log("cnt=%u C=%fpF\n", cnt, result.capacitance_pF);
    return true;
}

void cap_medium(unsigned int p0, unsigned int p1, unsigned int p2)
{
#if __ARM_EABI__
    static GPIO_Module *const r680_gpios[3] = { GPIOA, GPIOA, GPIOA };
    static const uint16_t r680_pins[3] = { GPIO_PIN_0, GPIO_PIN_4, GPIO_PIN_6 };
#endif
    debug_log("%s(%u, %u)\n", __FUNCTION__, p0, p1);
    probe_configure(p2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    //probe_discharge(p0, p1);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    tim6_msleep(100);
    const uint_fast8_t vref = 32;
    comp_init(p1, vref);
#if __ARM_EABI__
    r680_gpios[p1]->PBSC = r680_pins[p1];
#else
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
#endif
    uint32_t cnt = comp_wait(480000000 / 2);
    /* use counter value even if timeout reached */
    result.capacitance_pF = cnt * (1e12f / 48e6f / 691.0f / logf(63.0f / (63.0f - vref)));
    debug_log("cnt=%u C=%fpF\n", cnt, result.capacitance_pF);
}

void cap_big(unsigned int p0, unsigned int p1, unsigned int p2)
{
    static const unsigned int channels[3] = {1, 3, 7};
    debug_log("%s(%u, %u)\n", __FUNCTION__, p0, p1);
    probe_configure(p2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    float u = (adc_average(channels[p1], 100) - adc_average(channels[p0], 100)) * (5.0f / 4095.0f);
    if (u < 0.3f)
    {
        while (true)
        {
            u = (adc_average(channels[p1], 100) - adc_average(channels[p0], 100)) * (5.0f / 4095.0f);
            if (0.3f < u)
            {
                break;
            }
            IWDG_ReloadKey();
        }
    }

    probe_configure(p0, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p1, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    adc_average(channels[p0], 100);
    adc_average(channels[p1], 100);
    while (true)
    {
        u = (adc_average(channels[p1], 100) - adc_average(channels[p0], 100)) * (5.0f / 4095.0f);
        if (0.01f < u)
        {
            break;
        }
        IWDG_ReloadKey();
    }

    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(100);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    tim6_msleep(1000);
    comp_init(p1, 10);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    static const uint_fast32_t timeout = 48000000 / 10;
    uint32_t cnt = comp_wait(timeout);
    if (cnt >= timeout)
    {
        result.capacitance_pF = 3e10f; /* 30mF */
        return;
    }

    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    /*
        U_c(t) = U_in * (1 - e^(-t/RC))
        C = t / (R * ln((U_in - U_c) / U_in))
    */
    u = (adc_average(channels[p1], 100) - adc_average(channels[p0], 100)) * (5.0f / 4095.0f);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    result.capacitance_pF = cnt * (1e12f / 48e6f / 680.0f / logf(5.0f / (5.0f - u)));
    debug_log("cnt=%u U=%f C=%fpF\n", cnt, u, result.capacitance_pF);
}

bool cap_bat(void)
{
    static const unsigned int probes[3][3] =
    {
        {0, 1, 2},
        {0, 2, 1},
        {1, 2, 0},
    };
    cap_bat_find();
    debug_log("cap_bat_find %u\n", result.component);
    if (result.component == COMPONENT_BATTERY)
    {
        // measure_bat_voltage();
        debug_log("Battery\n");
    }
    else
    {
        if (result.component == COMPONENT_CAP)
        {
            cap_big(result.probes[0], result.probes[2], result.probes[1]);
            if (55e6f < result.capacitance_pF) /* >55uF? */
            {
                // cap_esr(result.probes[0], result.probes[2], true);
                // cap_vloss(result.probes[0], result.probes[2]);
                return true;
            }
        }
        for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
        {
            cap_medium(probes[i][0], probes[i][1], probes[i][2]);
            if (1e8f < result.capacitance_pF) /* >100uF? shouldn't happen */
            {
                result.capacitance_pF = 0.0f;
                return false;
            }
            if (90e3f < result.capacitance_pF) /* >90nF? */
            {
                result.component = COMPONENT_CAP;
                result.probes[0] = probes[i][0];
                result.probes[2] = probes[i][1];
                // cap_esr(result.probes[0], result.probes[2], true);
                // cap_vloss(result.probes[0], result.probes[2]);
                return true;
            }
        }
        float c_max = 0.0f;
        int max_idx;
        for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
        {
            cap_small(probes[i][0], probes[i][1], probes[i][2], true);
            if (!(c_max > result.capacitance_pF))
            {
                c_max = result.capacitance_pF;
                max_idx = i;
            }
        }
        if (c_max <= 0.0f)
        {
            return false;
        }
        result.capacitance_pF = c_max;
        result.component = COMPONENT_CAP;
        result.probes[0] = probes[max_idx][0];
        result.probes[2] = probes[max_idx][1];
    }
    return true;
}
