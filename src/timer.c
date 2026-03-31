#include "n32g031_tim.h"
#include "timer.h"

void tim6_init(void)
{
    TIM_TimeBaseInitType TIM_TimeBaseInitStruct; 
    TIM_DeInit(TIM6);
    TIM_TimeBaseInitStruct.Period = 0xffff;
    TIM_TimeBaseInitStruct.ClkDiv = TIM_CLK_DIV1;
    TIM_TimeBaseInitStruct.Prescaler = 48 - 1;
    TIM_TimeBaseInitStruct.CntMode = TIM_CNT_MODE_UP;
    TIM_InitTimeBase(TIM6, &TIM_TimeBaseInitStruct);
    TIM_Enable(TIM6, ENABLE);
}

void tim6_msleep(uint_fast32_t ms)
{
    while (ms--)
    {
        tim6_usleep(1000);
        IWDG_ReloadKey();
    }
}

void tim6_usleep(uint_fast16_t us)
{
    TIM6->CNT = 0;
    while (TIM6->CNT < us) { }
}
