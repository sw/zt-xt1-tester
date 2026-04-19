#include "n32g031_gpio.h"
#include "adc.h"
#include "component.h"
#include "globals.h"
#include "gpio.h"
#include "probe.h"
#include "timer.h"

static const uint_least8_t direct_pins[3] = { GPIO_PIN_1, GPIO_PIN_3, GPIO_PIN_7 };
static const uint_least8_t r680_pins[3]   = { GPIO_PIN_0, GPIO_PIN_4, GPIO_PIN_6 };

bool dht11_detect(void)
{
    static const unsigned int channels[3] = {1, 3, 7};
    static const unsigned int probes[][3] =
    {
        {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}
    };

    result.component = COMPONENT_NONE;
    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        result.probes[0] = probes[i][0];
        result.probes[1] = probes[i][1];
        result.probes[2] = probes[i][2];
        probe_configure(result.probes[0], PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
        probe_configure(result.probes[1], PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
        probe_configure(result.probes[2], PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
        tim6_msleep(1);
        uint_fast16_t a0 = adc_average(channels[result.probes[0]], 1);
        uint_fast16_t a1 = adc_average(channels[result.probes[1]], 1);
        uint_fast16_t a2 = adc_average(channels[result.probes[2]], 1);
        gpio_init(GPIOA, direct_pins[result.probes[1]], GPIO_MODE_INPUT, GPIO_PULL_UP);
        GPIO_WriteBit(GPIOA, r680_pins[result.probes[1]], Bit_SET);
        gpio_init(GPIOA, r680_pins[result.probes[1]], GPIO_MODE_OUTPUT_OD, GPIO_PULL_UP);
        if (   ((a0 > 3000) && (a0 < 4050))
            && (a1 > 3000)
            && (a2 < 50) )
        {
            result.component = COMPONENT_DHT11;
            result.temperature = -100.0f;
            return true;
        }
    }
    return false;
}

static bool dht11_error(void)
{
    result.component = COMPONENT_NONE;
    result.humidity = 0.0f;
    result.temperature = -100.0f;
    return false;
}

bool dht11_read(void)
{
    const uint_fast16_t r680_pin = r680_pins[result.probes[1]];
    const uint_fast16_t direct_pin = direct_pins[result.probes[1]];
    uint_least16_t bit_durations[40];
    uint_least8_t  octets[5];

    /* generate start condition */
    GPIO_WriteBit(GPIOA, r680_pin, Bit_RESET);
    tim6_msleep(20);
    GPIO_WriteBit(GPIOA, r680_pin, Bit_SET);

    /* wait for response: DHT pulls line low */
    TIM6->CNT = 0;
    while (GPIO_ReadOutputDataBit(GPIOA, direct_pin))
    {
        if (TIM6->CNT > 100)
        {
            return dht11_error();
        }
    }

    TIM6->CNT = 0;
    while (!GPIO_ReadOutputDataBit(GPIOA, direct_pin))
    {
        if (TIM6->CNT > 100)
        {
            return dht11_error();
        }
    }

    TIM6->CNT = 0;
    while (GPIO_ReadOutputDataBit(GPIOA, direct_pin))
    {
        if (TIM6->CNT > 100)
        {
            return dht11_error();
        }
    }

    TIM6->CNT = 0;
    for (int i = 0; i < 40; i++)
    {
        while (!GPIO_ReadOutputDataBit(GPIOA, direct_pin))
        {
            if (TIM6->CNT > 100)
            {
                return false;
            }
        }
        TIM6->CNT = 0;
        while (GPIO_ReadOutputDataBit(GPIOA, direct_pin))
        {
            if (TIM6->CNT > 100)
            {
                return false;
            }
        }
        bit_durations[i] = TIM6->CNT;
        TIM6->CNT = 0;
    }
    for (int i = 0; i < 5; i++)
    {
        uint32_t u = 0;
        octets[i] = 0;
        for (int bit = 0; bit < 8; bit++)
        {
            u = (u << 25) >> 24;
            octets[i] = u;
            if (bit_durations[8 * i + bit] > 50)
            {
                u++;
                octets[i] = u;
            }
        }
    }
    result.humidity    = (float)octets[0];
    result.temperature = (float)octets[2];
    /* TODO: this seems wrong, but DHT11 doesn't officially support negative temperatures anyway */
    if ((int8_t)octets[3] < 0)
    {
        octets[3] &= 0xF;
        result.temperature = -(octets[2] + octets[3] / 10.0f);
    }
    else
    {
        result.temperature = octets[2] + octets[3] / 10.0f;
    }
    return true;
}
