#pragma once

#include "n32g031_gpio.h"

void gpio_init(GPIO_Module *gpio, uint_fast16_t pin, uint_fast32_t mode, uint_fast32_t pull);
