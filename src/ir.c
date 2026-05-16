#include <string.h>
#include "n32g031_gpio.h"
#include "adc.h"
#include "component.h"
#include "globals.h"
#include "gpio.h"
#include "probe.h"

static const uint_least8_t direct_pins[3] = { GPIO_PIN_1, GPIO_PIN_3, GPIO_PIN_7 };

bool ir_detect(void)
{
    static const unsigned int channels[3] = {1, 3, 7};
    static const unsigned int probes[][3] =
    {
        {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}
    };

    result.component = COMPONENT_NONE;
    result.current_mA = 0.0f;
    result.ir_a[0] = 0.0f;
    result.ir_a[1] = 0.0f;
    result.ir_a[2] = 0.0f;
    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        result.probes[0] = probes[i][0];
        result.probes[1] = probes[i][1];
        result.probes[2] = probes[i][2];
        probe_configure(result.probes[0], PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
        probe_configure(result.probes[1], PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
        probe_configure(result.probes[2], PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
        uint_fast16_t a0 = adc_average(channels[result.probes[0]], 1);
        uint_fast16_t a1 = adc_average(channels[result.probes[1]], 1);
        uint_fast16_t a2 = adc_average(channels[result.probes[2]], 1);
        if ((4030 < a0) && (a1 < 20) && (3500 < a2) && (a2 < 4030))
        {
            result.component = COMPONENT_INFRARED;

            static const uint_least8_t r680_pins[3] = { GPIO_PIN_0, GPIO_PIN_4, GPIO_PIN_6 };

            /* ~33kohm from VS1838B OUT to VCC */
            probe_configure(result.probes[0], PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
            probe_configure(result.probes[1], PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
            probe_configure(result.probes[2], PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
            gpio_init(GPIOA, direct_pins[result.probes[0]], GPIO_MODE_INPUT, GPIO_PULL_UP);
            gpio_init(GPIOA, r680_pins[result.probes[0]], GPIO_MODE_INPUT, GPIO_PULL_UP);
            return true;
        }
    }
    return false;
}

void ir_read(void)
{
    /* This buffer is 1072 bytes in the original firmware - why? */
    static struct
    {
        uint32_t result;                    // 200000C4
        uint32_t result16_0;                // 200000C8
        uint32_t result16_1;                // 200000CC
        uint32_t pulse_duration[32 * 2];    // 200000D0
    } buf;

    if (result.component != COMPONENT_INFRARED)
    {
        return;
    }
    if (GPIO_ReadOutputDataBit(GPIOA, direct_pins[result.probes[0]]))
    {
        return; /* output high = idle -> loop in main */
    }

    TIM6->CNT = 0;
    memset(&buf, 0, sizeof(buf));

    /* leading pulse: 9ms */
    while (!GPIO_ReadOutputDataBit(GPIOA, direct_pins[result.probes[0]]))
    {
        if (TIM6->CNT > 10000)
        {
            return;
        }
    }

    /* space: 4.5ms */
    TIM6->CNT = 0;
    while (GPIO_ReadOutputDataBit(GPIOA, direct_pins[result.probes[0]]))
    {
        if (TIM6->CNT > 5000)
        {
            return;
        }
    }

    /* read 4 bytes */
    TIM6->CNT = 0;
    for (int i = 0; i < 32; i++)
    {
        while (!GPIO_ReadOutputDataBit(GPIOA, direct_pins[result.probes[0]]))
        {
            if (TIM6->CNT > 2000)
            {
                return;
            }
        }
        buf.pulse_duration[i * 2] = TIM6->CNT;
        TIM6->CNT = 0;
        while (GPIO_ReadOutputDataBit(GPIOA, direct_pins[result.probes[0]]))
        {
            if (TIM6->CNT > 2000)
            {
                return;
            }
        }
        buf.pulse_duration[i * 2 + 1] = TIM6->CNT;
        TIM6->CNT = 0;
    }

    for (int i = 0; i < 32; i++)
    {
        buf.result >>= 1;
        if ((buf.pulse_duration[i * 2] << 1) < (buf.pulse_duration[i * 2 + 1]))
        {
            buf.result += 0x80000000;
        }
    }
    /* weird endianness conversion */
    buf.result16_0 = ((buf.result << 24) >> 16) + ((buf.result << 16) >> 24);
    buf.result16_1 = ((buf.result << 8) >> 24) * 0x100 + (buf.result >> 24);

    ir_decoded = true;
    result.current_mA = 1.0f;
    /* WTF??? */
    result.ir_a[0] = (float)buf.result;
    result.ir_a[1] = (float)buf.result16_0;
    result.ir_a[2] = (float)buf.result16_1;
}
