#include "n32g031_gpio.h"
#include "component.h"
#include "globals.h"
#include "gpio.h"
#include "probe.h"
#include "timer.h"

enum : uint8_t
{
    COMMAND_ROM_READ        = 0x33,
    COMMAND_T_CONVERT       = 0x44,
    COMMAND_SCRATCHPAD_READ = 0xBE,
    COMMAND_ROM_SKIP        = 0xCC,
};

static const uint_least8_t direct_pins[3] = { GPIO_PIN_1, GPIO_PIN_3, GPIO_PIN_7 };
static const uint_least8_t r680_pins[3]   = { GPIO_PIN_0, GPIO_PIN_4, GPIO_PIN_6 };

static bool ds18b20_init(void)
{
    const uint_fast16_t r680_pin = r680_pins[result.probes[1]];
    const uint_fast16_t direct_pin = direct_pins[result.probes[1]];

    /* master reset pulse */
    GPIO_WriteBit(GPIOA, r680_pin, Bit_RESET);
    tim6_usleep(500);
    GPIO_WriteBit(GPIOA, r680_pin, Bit_SET);

    /* look for DS18B20 presence pulse */
    tim6_usleep(60);
    if (!GPIO_ReadOutputDataBit(GPIOA, direct_pin))
    {
        TIM6->CNT = 0;
        do
        {
            if (GPIO_ReadOutputDataBit(GPIOA, direct_pin))
            {
                tim6_usleep(500);
                return true;
            }
        }
        while (TIM6->CNT < 501);
    }
    else
    {
        tim6_usleep(500);
    }
    return false;
}

static void ds18b20_write_byte(uint_fast8_t data)
{
    const uint_fast16_t r680_pin = r680_pins[result.probes[1]];

    for (int i = 0; i < 8; i++)
    {
        GPIO_WriteBit(GPIOA, r680_pin, Bit_RESET);
        tim6_usleep(5);
        if (!(data & 1))
        {
            tim6_usleep(70);
            GPIO_WriteBit(GPIOA, r680_pin, Bit_SET);
        }
        else
        {
            GPIO_WriteBit(GPIOA, r680_pin, Bit_SET);
            tim6_usleep(70);
        }
        tim6_usleep(5);
        data >>= 1;
    }
}

static uint_fast8_t ds18b20_read_byte(void)
{
    const uint_fast16_t r680_pin = r680_pins[result.probes[1]];
    const uint_fast16_t direct_pin = direct_pins[result.probes[1]];

    uint_fast8_t byte = 0;
    for (int i = 0; i < 8; i++)
    {
        byte >>= 1;
        GPIO_WriteBit(GPIOA, r680_pin, Bit_RESET);
        tim6_usleep(5);
        GPIO_WriteBit(GPIOA, r680_pin, Bit_SET);
        tim6_usleep(5);
        if (GPIO_ReadOutputDataBit(GPIOA, direct_pin))
        {
            byte |= 0x80;
        }
        tim6_usleep(70);
    }
    return byte;
}

bool ds18b20_detect(void)
{
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
        probe_configure(result.probes[0], PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
        probe_configure(result.probes[1], PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
        probe_configure(result.probes[2], PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
        gpio_init(GPIOA, direct_pins[result.probes[1]], GPIO_MODE_INPUT, GPIO_PULL_UP);
        gpio_init(GPIOA, r680_pins[result.probes[1]], GPIO_MODE_OUTPUT_PP, GPIO_PULL_UP);
        tim6_msleep(1);

        __disable_irq();
        ds18b20_init();
        ds18b20_write_byte(COMMAND_ROM_READ);
        result.ds18b20_rom_code[0] = ds18b20_read_byte();
        /* correct family code? */
        if (result.ds18b20_rom_code[0] == 0x28)
        {
            for (int j = 1; j < 8; j++)
            {
                result.ds18b20_rom_code[j] = ds18b20_read_byte();
            }

            ds18b20_init();
            ds18b20_write_byte(COMMAND_ROM_SKIP);
            ds18b20_write_byte(COMMAND_T_CONVERT);
            __enable_irq();
            result.component = COMPONENT_DS18B20;
            result.temperature = -100.0f;
            return true;
        }
        __enable_irq();
    }
    return false;
}

bool ds18b20_read(void)
{
    union
    {
        uint8_t byte[2];
        int16_t temperature;
    } buf;

    __disable_irq();
    if (ds18b20_init())
    {
        ds18b20_write_byte(COMMAND_ROM_SKIP);
        ds18b20_write_byte(COMMAND_SCRATCHPAD_READ);
        buf.byte[0] = ds18b20_read_byte();
        buf.byte[1] = ds18b20_read_byte();

        if (ds18b20_init())
        {
            ds18b20_write_byte(COMMAND_ROM_SKIP);
            ds18b20_write_byte(COMMAND_T_CONVERT);
            __enable_irq();
            /* TODO: could FFFF be a valid temperature just below 0˚C? -> better check CRC instead */
            if ((buf.byte[0] != 0xFF) || (buf.byte[1] != 0xFF))
            {
                result.temperature = buf.temperature * 0.0625f;
                return true;
            }
        }
    }
    __enable_irq();
    result.component = COMPONENT_NONE;
    result.temperature = -100.0f;
    return false;
}
