#include "n32g031_gpio.h"

void gpio_init(GPIO_Module *gpio, uint_fast16_t pin, uint_fast32_t mode, uint_fast32_t pull)
{
  GPIO_InitType GPIO_InitStruct;
  GPIO_InitStruct.GPIO_Alternate = GPIO_NO_AF;
  GPIO_InitStruct.GPIO_Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.GPIO_Current = GPIO_DC_HIGH;
  GPIO_InitStruct.Pin = pin;
  GPIO_InitStruct.GPIO_Mode = mode;
  GPIO_InitStruct.GPIO_Pull = pull;
  GPIO_InitPeripheral(gpio, &GPIO_InitStruct);
}
