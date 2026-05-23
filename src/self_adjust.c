#include <string.h>
#ifdef __ARM_EABI__
#include "n32g031_flash.h"
#else
#include <cmocka.h>
#endif
#include "globals.h"
#include "self_adjust.h"

#define MAGIC 0x12345678
#define FLASH_ADDR 0x0800fe00

typedef union
{
    struct
    {
        uint32_t magic0;
        self_adjust_values_t val;
        uint32_t magic1;
    };
    uint32_t raw[10];
} self_adjust_flash_t;
static_assert(sizeof(self_adjust_flash_t) == 40);

bool self_adjust_load(void)
{
    self_adjust_vals = ((self_adjust_flash_t *)FLASH_ADDR)->val;
    return (   (((self_adjust_flash_t *)FLASH_ADDR)->magic0 == MAGIC)
            && (((self_adjust_flash_t *)FLASH_ADDR)->magic1 == MAGIC) );
}

void self_adjust_default(void)
{
    self_adjust_vals.rp = 15.0f;
    self_adjust_vals.rd = 15.0f;
    self_adjust_vals.probe12_cap = 26.28f;
    self_adjust_vals.probe13_cap = 31.97f;
    self_adjust_vals.probe21_cap = 28.04f;
    self_adjust_vals.probe23_cap = 31.97f;
    self_adjust_vals.probe31_cap = 27.93f;
    self_adjust_vals.probe32_cap = 26.70f;
}

void self_adjust_write(void)
{
#ifdef __ARM_EABI__
    self_adjust_flash_t buf;
    buf.val = self_adjust_vals;
    buf.magic0 = MAGIC;
    buf.magic1 = MAGIC;
    FLASH_Unlock();
    FLASH_EraseOnePage(FLASH_ADDR);
    for (int i = 0; i < sizeof(buf.raw) / sizeof(uint32_t); i++)
    {
        FLASH_ProgramWord(FLASH_ADDR + i * sizeof(uint32_t), buf.raw[i]);
    }
    FLASH_Lock();
#else
    function_called();
#endif
}
