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

bool cap_small(unsigned int p0, unsigned int p1, unsigned int p2_unused, bool subtract_probe)
{
#if __ARM_EABI__
    static GPIO_Module *const r470k_gpios[3] = { GPIOA, GPIOA, GPIOB };
    static const uint16_t r470k_pins[3] = { GPIO_PIN_2, GPIO_PIN_5, GPIO_PIN_1 };
#else
    /* TODO: simulation */
    return false;
#endif

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

    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_DRV_LO);
    tim6_msleep(2);
    const uint_fast8_t vref = 51;
    comp_init(p1, vref);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_LO);
#if __ARM_EABI__
    r470k_gpios[p1]->PBSC = r470k_pins[p1];
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
    return true;
}

void cap_medium(unsigned int p0, unsigned int p1, unsigned int p2)
{
#if __ARM_EABI__
    static GPIO_Module *const r680_gpios[3] = { GPIOA, GPIOA, GPIOA };
    static const uint16_t r680_pins[3] = { GPIO_PIN_0, GPIO_PIN_4, GPIO_PIN_6 };
#endif
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
#endif
    uint32_t cnt = comp_wait(480000000 / 2);
    /* use counter value even if timeout reached */
    result.capacitance_pF = cnt * (1e12f / 48e6f / 691.0f / logf(63.0f / (63.0f - vref)));
}

void cap_big(unsigned int p0, unsigned int p1, unsigned int p2)
{
    static const unsigned int channels[3] = {1, 3, 7};
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
}
