#include <string.h>
#include "n32g031_flash.h"
#include "calib.h"
#include "globals.h"

#define MAGIC 0x12345678
#define FLASH_ADDR 0x0800fe00

bool calib_load(void)
{
    memcpy(&calibration, (void *)FLASH_ADDR, sizeof(calibration));
    return (calibration.magic0 == MAGIC) && (calibration.magic1 == MAGIC);
}

void calib_default(void)
{
    memset(&calibration, 0, sizeof(calibration));
    calibration.magic0 = MAGIC;
    calibration.rp = 15.0f;
    calibration.magic1 = MAGIC;
    calibration.rd = 15.0f;
    calibration.probe12_cap = 26.28f;
    calibration.probe13_cap = 31.97f;
    calibration.probe21_cap = 28.04f;
    calibration.probe23_cap = 31.97f;
    calibration.probe31_cap = 27.93f;
    calibration.probe32_cap = 26.70f;
}

void calib_write(void)
{
    FLASH_Unlock();
    FLASH_EraseOnePage(FLASH_ADDR);
}
