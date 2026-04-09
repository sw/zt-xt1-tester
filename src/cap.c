#include <math.h>
#ifdef __ARM_EABI__
#include "n32g031_gpio.h"
#include "n32g031_tim.h"
#endif
#include "adc.h"
#include "cap.h"
#include "comp.h"
#include "debug.h"
#include "globals.h"
#include "helpers.h"
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
            /*
                Charging a 55uF capacitor for 10ms through 680ohms from 0V takes it to 1.17V.
                Bigger capacitors will have a lower voltage.
            */
            if (u < 2.0f)
            {
                tim6_msleep(1000);
                float u2 = adc_average(channels[probes[i][1]], 100) * (5.0f / 4095.0f);
                /* in 1s, a 100mF capacitor will charge to 73mV */
                if (u2 > u + 0.05f)
                {
                    debug_log("%s found capacitor\n", __FUNCTION__);
                    result.component = COMPONENT_CAP;
                    break;
                }
            }
        }
        else
        {
            /* voltage was high, could be a charged capacitor or a battery -> discharge for 10ms through 680ohms */
            probe_configure(probes[i][1], PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
            tim6_msleep(10);
            u = adc_average(channels[probes[i][1]], 100) * (5.0f / 4095.0f);
            if (u > 0.5f)
            {
                tim6_msleep(1000);
                float u2 = adc_average(channels[probes[i][1]], 100) * (5.0f / 4095.0f);
                /* after 1 second, a 100mF capacitor starting from 5V will have lost 73mV */
                /* for a battery, this is a charge loss of ~1uAh which won't drop the voltage as much */
                if (u2 + 0.05f > u)
                {
                    debug_log("%s found battery\n", __FUNCTION__);
                    result.component = COMPONENT_BATTERY;
                }
                else
                {
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

bool cap_small(unsigned int p0, unsigned int p1, unsigned int p2, bool subtract_probe)
{
#ifdef __ARM_EABI__
    static GPIO_Module *const r470k_gpios[3] = { GPIOA, GPIOA, GPIOB };
    static const uint16_t r470k_pins[3] = { GPIO_PIN_2, GPIO_PIN_5, GPIO_PIN_1 };
#else
    static const unsigned int r470k_gpios[3] = { 0 };
    static const unsigned int r470k_pins[3] = { 2, 2, 2 };
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

    /*
        p2 is not initialised in the original firmware, leading to wrong values or probe numbering.
        We actually need to pull it low first, to discharge the capacitor in case it's connected here.
    */
    probe_configure(p2, PROBE_ANALOG, PROBE_DRV_LO, PROBE_DRV_LO);
    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_DRV_LO);
    tim6_msleep(2);
    const uint_fast8_t vref = 51;
    comp_init(p1, vref);

    /* not in original firmware: reset p2 after discharging */
    probe_configure(p2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);

    /* maximum value of 90nF would need ~3.4 megaticks */
    static const uint_fast32_t timeout = 48000000 / 10;
    uint32_t cnt = comp_start(r470k_gpios[p1], r470k_pins[p1], timeout);

    /* probes are not reset in the original firmware */
    probe_configure(p0, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);

    if (cnt >= timeout)
    {
        result.capacitance_pF = 0.0f;
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
    debug_log("cnt=%u C=%.1fpF\n", cnt, result.capacitance_pF);
    return true;
}

#ifdef __ARM_EABI__
static GPIO_Module *const r680_gpios[3] = { GPIOA, GPIOA, GPIOA };
static const uint16_t r680_pins[3] = { GPIO_PIN_0, GPIO_PIN_4, GPIO_PIN_6 };
#else
static const unsigned int r680_gpios[3] = { 0 };
static const unsigned int r680_pins[3] = { 1, 1, 1 };
#endif

static void cap_medium(unsigned int p0, unsigned int p1, unsigned int p2)
{
    debug_log("%s(%u, %u)\n", __FUNCTION__, p0, p1);
    probe_configure(p2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_discharge(p0, p1);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    tim6_msleep(100);
    const uint_fast8_t vref = 32;
    comp_init(p1, vref);
    /* original firmware has 4.8M but only 1.3M are needed for maximum value of 55uF */
    uint32_t cnt = comp_start(r680_gpios[p1], r680_pins[p1], 48000000 / 2);
    /* use counter value even if timeout reached */
    result.capacitance_pF = cnt * (1e12f / 48e6f / 691.0f / logf(63.0f / (63.0f - vref)));
    debug_log("cnt=%u C=%.1fnF\n", cnt, result.capacitance_pF / 1e3f);

    /* probes are not reset in the original firmware */
    probe_configure(p0, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
}

static void cap_big(unsigned int p0, unsigned int p1, unsigned int p2)
{
    static const unsigned int channels[3] = {1, 3, 7};
    debug_log("%s(%u, %u)\n", __FUNCTION__, p0, p1);
    probe_configure(p2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
#ifndef __ARM_EABI__
    tim6_usleep(10); /* not in original firmware, required for simulation */
#endif
    float u = (adc_average(channels[p1], 100) - adc_average(channels[p0], 100)) * (5.0f / 4095.0f);
    if (u < 0.3f)
    {
        while (true)
        {
#ifndef __ARM_EABI__
            tim6_usleep(10); /* not in original firmware, required for simulation */
#endif
            u = (adc_average(channels[p1], 100) - adc_average(channels[p0], 100)) * (5.0f / 4095.0f);
            if (0.3f < u)
            {
                break;
            }
            iwdg_reload();
        }
    }

    probe_configure(p0, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p1, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
#ifndef __ARM_EABI__
    tim6_usleep(10); /* not in original firmware, required for simulation */
#endif
    adc_average(channels[p0], 100);
    adc_average(channels[p1], 100);
    while (true)
    {
#ifndef __ARM_EABI__
        tim6_usleep(10); /* not in original firmware, required for simulation */
#endif
        u = (adc_average(channels[p1], 100) - adc_average(channels[p0], 100)) * (5.0f / 4095.0f);
        if (0.01f < u)
        {
            break;
        }
        iwdg_reload();
    }

    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(100);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    tim6_msleep(1000);
    comp_init(p1, 10);
    /* TODO: this doesn't quite fit the comp_start() semantics */
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    static const uint_fast32_t timeout = 48000000 * 5;
    uint32_t cnt = comp_start(r680_gpios[p1], r680_pins[p1], timeout);
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
    debug_log("cnt=%u U=%f C=%.1fuF\n", cnt, u, result.capacitance_pF / 1e6f);
}

static void cap_esr(unsigned int p0, unsigned int p1, bool discharge)
{
    static const unsigned int channels[3] = {1, 3, 7};
    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, p0, p1, discharge);
    if (discharge)
    {
        probe_discharge(p0, p1);
    }
    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(100);
    float r;
    /* original firmware does this 10 times but throws away the first 9 results */
    //for (int i = 0; i < 10; i++)
    {
        int_fast32_t u0_sum = 0;
        int_fast32_t u1_sum = 0;
        for (int j = 0; j < 500; j++)
        {
            iwdg_reload();
            probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
            probe_configure(p1, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
            /* 78us until p1 goes hi-Z */
#ifndef __ARM_EABI__
            tim6_usleep(30);    /* adjust simulation timing to original firmware */
#endif
            u1_sum += adc_single(channels[p1]);
#ifndef __ARM_EABI__
            tim6_usleep(30);    /* adjust simulation timing to original firmware */
#endif
            u0_sum += adc_single(channels[p0]);
            tim6_usleep(4
#ifndef __ARM_EABI__
                + 18   /* adjust simulation timing to original firmware */
#endif
            );
            probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
#ifndef __ARM_EABI__
            tim6_usleep(43);    /* adjust simulation timing to original firmware */
#endif
            adc_single(channels[p1]); /* result thrown away */
            probe_configure(p1, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
            tim6_usleep(990
#ifndef __ARM_EABI__
                + 90            /* adjust simulation timing to original firmware */
#endif
            );
        }
        r = (u1_sum - u0_sum) * (30.0f / 1.05f * 0.125f) / u0_sum;
    }
    debug_log("ESR=%.3f\n", r);
    result.resistance = r;
}

static void cap_vloss(unsigned int p0, unsigned int p1)
{
    static const unsigned int channels[3] = {1, 3, 7};
    unsigned int num;
    float u_charge;
    if (result.capacitance_pF < 18e6f)
    {
        /* capacitance < 18uF -> charge to 2.5V 50 times */
        num = 50;
        u_charge = 2.5f;
    }
    else
    {
        /* this seems a bad choice, giving values of >7% instead of aroung 2..3% for <18uF */
        u_charge = 1.25f;
        if (result.capacitance_pF < 700e6f)
        {
            /* capacitance < 700uF -> charge to 1.25V 5 times */
            num = 5;
        }
        else
        {
            /* capacitance > 700uF -> charge to 1.25V one time */
            num = 1;
        }
    }
    float sum = 0.0;
    for (int i = 0; i < num; i++)
    {
        probe_discharge(p0, p1);
        probe_configure(p1, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
        float u_before = 0.0f;
        while (u_before < u_charge)
        {
#ifndef __ARM_EABI__
            tim6_usleep(10); /* not in original firmware, required for simulation */
#endif
            /* here, 2^12 is actually used instead of 2^12-1. go figure... */
            u_before = adc_average(channels[p1], 1) * (5.0f / 4096.0f);
            iwdg_reload();
        }
        probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
        tim6_usleep(10);
        float u_after = adc_average(channels[p1], 1) * (5.0f / 4096.0f);
        sum += (u_before - u_after) * 100.0f / u_before;
    }
    result.cap_vloss = sum / num;
    debug_log("Vloss=%.2f%%\n", result.cap_vloss);
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
                cap_esr(result.probes[0], result.probes[2], true);
                cap_vloss(result.probes[0], result.probes[2]);
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
                /*
                    For values below 700nF, the ESR is overwritten by the subsequent resistor measurement.
                    But the ESR value is not displayed below 700nF.
                */
                cap_esr(result.probes[0], result.probes[2], true);
                cap_vloss(result.probes[0], result.probes[2]);
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
        debug_log("found cap C=%.0fpF\n", c_max);
        result.capacitance_pF = c_max;
        result.component = COMPONENT_CAP;
        result.probes[0] = probes[max_idx][0];
        result.probes[2] = probes[max_idx][1];
    }
    return true;
}
