#include "debug.h"
#include "gpio.h"
#include "probe.h"

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
