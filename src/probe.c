#include "adc.h"
#include "debug.h"
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
