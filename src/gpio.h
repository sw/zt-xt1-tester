#pragma once

#include "n32g031_gpio.h"

void gpio_init(GPIO_Module *gpio, uint32_t pin, uint32_t mode, uint32_t pull);
