#include <math.h>
#include "adc.h"
#include "component.h"
#include "debug.h"
#include "globals.h"
#include "helpers.h"
#include "timer.h"
#include "probe.h"

#ifdef __ARM_EABI__
#include "gpio.h"

void probe_configure(uint_fast8_t probe, probe_mode_t direct, probe_mode_t r680, probe_mode_t r470k)
{
    if (probe == 0)
    {
        if (direct == PROBE_ANALOG)
        {
            gpio_init(GPIOA, GPIO_PIN_1, GPIO_MODE_ANALOG, GPIO_NO_PULL);
        }
        else
        {
            assert((direct == PROBE_DRV_LO) || (direct == PROBE_DRV_HI));
            GPIO_WriteBit(GPIOA, GPIO_PIN_1, direct == PROBE_DRV_HI);
            gpio_init(GPIOA, GPIO_PIN_1, GPIO_MODE_OUTPUT_PP, GPIO_NO_PULL);
        }

        if (r680 == PROBE_ANALOG)
        {
            gpio_init(GPIOA, GPIO_PIN_0, GPIO_MODE_ANALOG, GPIO_NO_PULL);
        }
        else
        {
            assert((r680 == PROBE_DRV_LO) || (r680 == PROBE_DRV_HI));
            GPIO_WriteBit(GPIOA, GPIO_PIN_0, r680 == PROBE_DRV_HI);
            gpio_init(GPIOA, GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_NO_PULL);
        }

        if (r470k == PROBE_ANALOG)
        {
            gpio_init(GPIOA, GPIO_PIN_2, GPIO_MODE_ANALOG, GPIO_NO_PULL);
        }
        else
        {
            assert((r470k == PROBE_DRV_LO) || (r470k == PROBE_DRV_HI));
            GPIO_WriteBit(GPIOA, GPIO_PIN_2, r470k == PROBE_DRV_HI);
            gpio_init(GPIOA, GPIO_PIN_2, GPIO_MODE_OUTPUT_PP, GPIO_NO_PULL);
        }
    }
    else if (probe == 1)
    {
        if (direct == PROBE_ANALOG)
        {
            gpio_init(GPIOA, GPIO_PIN_3, GPIO_MODE_ANALOG, GPIO_NO_PULL);
        }
        else
        {
            assert((direct == PROBE_DRV_LO) || (direct == PROBE_DRV_HI));
            GPIO_WriteBit(GPIOA, GPIO_PIN_3, direct == PROBE_DRV_HI);
            gpio_init(GPIOA, GPIO_PIN_3, GPIO_MODE_OUTPUT_PP, GPIO_NO_PULL);
        }

        if (r680 == PROBE_ANALOG)
        {
            gpio_init(GPIOA, GPIO_PIN_4, GPIO_MODE_ANALOG, GPIO_NO_PULL);
        }
        else
        {
            assert((r680 == PROBE_DRV_LO) || (r680 == PROBE_DRV_HI));
            GPIO_WriteBit(GPIOA, GPIO_PIN_4, r680 == PROBE_DRV_HI);
            gpio_init(GPIOA, GPIO_PIN_4, GPIO_MODE_OUTPUT_PP, GPIO_NO_PULL);
        }

        if (r470k == PROBE_ANALOG)
        {
            gpio_init(GPIOA, GPIO_PIN_5, GPIO_MODE_ANALOG, GPIO_NO_PULL);
        }
        else
        {
            assert((r470k == PROBE_DRV_LO) || (r470k == PROBE_DRV_HI));
            GPIO_WriteBit(GPIOA, GPIO_PIN_5, r470k == PROBE_DRV_HI);
            gpio_init(GPIOA, GPIO_PIN_5, GPIO_MODE_OUTPUT_PP, GPIO_NO_PULL);
        }
    }
    else
    {
        assert(probe == 2);
        if (direct == PROBE_ANALOG)
        {
            gpio_init(GPIOA, GPIO_PIN_7, GPIO_MODE_ANALOG, GPIO_NO_PULL);
        }
        else
        {
            assert((direct == PROBE_DRV_LO) || (direct == PROBE_DRV_HI));
            GPIO_WriteBit(GPIOA, GPIO_PIN_7, direct == PROBE_DRV_HI);
            gpio_init(GPIOA, GPIO_PIN_7, GPIO_MODE_OUTPUT_PP, GPIO_NO_PULL);
        }

        if (r680 == PROBE_ANALOG)
        {
            gpio_init(GPIOA, GPIO_PIN_6, GPIO_MODE_ANALOG, GPIO_NO_PULL);
        }
        else
        {
            assert((r680 == PROBE_DRV_LO) || (r680 == PROBE_DRV_HI));
            GPIO_WriteBit(GPIOA, GPIO_PIN_6, r680 == PROBE_DRV_HI);
            gpio_init(GPIOA, GPIO_PIN_6, GPIO_MODE_OUTPUT_PP, GPIO_NO_PULL);
        }

        if (r470k == PROBE_ANALOG)
        {
            gpio_init(GPIOB, GPIO_PIN_1, GPIO_MODE_ANALOG, GPIO_NO_PULL);
        }
        else
        {
            assert((r470k == PROBE_DRV_LO) || (r470k == PROBE_DRV_HI));
            GPIO_WriteBit(GPIOB, GPIO_PIN_1, r470k == PROBE_DRV_HI);
            gpio_init(GPIOB, GPIO_PIN_1, GPIO_MODE_OUTPUT_PP, GPIO_NO_PULL);
        }
    }
}
#endif /* __ARM_EABI__ */

void probe_discharge(uint_fast8_t p0, uint_fast8_t p1)
{
    static const unsigned int channels[3] = {1, 3, 7};

    probe_configure(p0, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p1, PROBE_DRV_LO, PROBE_DRV_LO, PROBE_ANALOG);
    adc_average(channels[p1], 100); /* measurement thrown away */
    float u;

    do
    {
        iwdg_reload();
#ifndef __ARM_EABI__
        tim6_usleep(10); /* not in original firmware, required for simulation */
#endif
        u = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
    }
    while (0.01f < u);

    probe_configure(p1,PROBE_ANALOG,PROBE_DRV_LO,PROBE_ANALOG);

    do
    {
        iwdg_reload();
        tim6_msleep(10);
        u = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
    }
    while (0.01f < u);
}

static bool probe_all_shorted_inner(unsigned int p0, unsigned int p1, unsigned int p2)
{
    static const unsigned int channels[3] = {1, 3, 7};

    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, p0, p1, p2);

    probe_configure(p0, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    float u0 = adc_average(channels[p0], 100) * (5.0f / 4095.0f);
    float u1 = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
    float u2 = adc_average(channels[p2], 100) * (5.0f / 4095.0f);
    if ((u0 > 0.1f) || (u1 > 0.1f) || (u2 > 0.1f))
    {
        return false;
    }

    probe_configure(p0, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(p2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    u0 = adc_average(channels[p0], 100) * (5.0f / 4095.0f);
    u1 = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
    u2 = adc_average(channels[p2], 100) * (5.0f / 4095.0f);
    if ((u0 < 4.8f) || (u1 < 4.8f) || (u2 < 4.8f))
    {
        return false;
    }

    probe_configure(p0, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(p2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    u0 = adc_average(channels[p0], 100) * (5.0f / 4095.0f);
    u1 = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
    u2 = adc_average(channels[p2], 100) * (5.0f / 4095.0f);
    if (   (u0 < 2.4f) || (u0 > 2.6f)
        || (u1 < 2.4f) || (u1 > 2.6f)
        || (u2 < 2.4f) || (u2 > 2.6f) )
    {
        return false;
    }

    return true;
}

bool probe_all_shorted(void)
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
        if (!probe_all_shorted_inner(probes[i][0], probes[i][1], probes[i][2]))
        {
            return false;
        }
    }
    return true;
}

void probe_calibrate_resistance(void)
{
    static const unsigned int channels[3] = {1, 3, 7};
    static const unsigned int probes[][3] =
    {
        {0, 1, 2},
        {0, 2, 1},
        {1, 0, 2},
        {1, 2, 0},
        {2, 0, 1},
        {2, 1, 0},
    };

    float r680_sum = 0.0f;
    float r0_sum   = 0.0f;
    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        probe_configure(probes[i][0], PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
        probe_configure(probes[i][1], PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
        probe_configure(probes[i][2], PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
        tim6_msleep(1);
        r680_sum += adc_average(channels[probes[i][2]], 500);
        probe_configure(probes[i][0], PROBE_DRV_HI, PROBE_ANALOG, PROBE_ANALOG);
        tim6_msleep(1);
        r0_sum += adc_average(channels[probes[i][2]], 500);
    }
    probe_configure(0, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);

    float r0_avg   = r0_sum   / 6.0f;
    float r680_avg = r680_sum / 6.0f;
    float rd = 680.0f / (4095.0f / r680_avg - 4095.0f / r0_avg);
    float rp = rd * (4095.0f - r0_avg) / r0_avg;

    if ((fabsf(rd) < 30.0f) && (fabsf(rp) < 30.0f))
    {
        self_adjust_vals.rp = rp;
        self_adjust_vals.rd = rd;
    }
    else
    {
        self_adjust_vals.rp = 15.0f;
        self_adjust_vals.rd = 15.0f;
    }
}

static bool probe_all_open_inner(unsigned int p0, unsigned int p1, unsigned int p2)
{
    static const unsigned int channels[3] = {1, 3, 7};

    debug_log("%s(%u, %u, %u)\n", __FUNCTION__, p0, p1, p2);

    probe_configure(p0, PROBE_ANALOG, PROBE_DRV_LO, PROBE_ANALOG);
    probe_configure(p1, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(p2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    tim6_msleep(10);
    float u0 = adc_average(channels[p0], 100) * (5.0f / 4095.0f);
    float u1 = adc_average(channels[p1], 100) * (5.0f / 4095.0f);
    if ((0.1f < u0) || (u1 < 4.8f))
    {
        return false;
    }
    return true;
}

bool probe_all_open(void)
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
        if (!probe_all_open_inner(probes[i][0], probes[i][1], probes[i][2]))
        {
            return false;
        }
    }
    return true;
}

void probe_calibrate_capacitance(void)
{
#ifndef __ARM_EABI__
    tim6_usleep(100); /* not in original firmware, required for simulation */
#endif
    cap_small(0, 1, 2, false);
    self_adjust_vals.probe12_cap = result.capacitance_pF;
    cap_small(0, 2, 1, false);
    self_adjust_vals.probe13_cap = result.capacitance_pF;
    cap_small(1, 0, 2, false);
    self_adjust_vals.probe21_cap = result.capacitance_pF;
    cap_small(1, 2, 0, false);
    self_adjust_vals.probe23_cap = result.capacitance_pF;
    cap_small(2, 0, 1, false);
    self_adjust_vals.probe31_cap = result.capacitance_pF;
    cap_small(2, 1, 0, false);
    self_adjust_vals.probe32_cap = result.capacitance_pF;

    probe_configure(0, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
}
