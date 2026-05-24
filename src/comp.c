#include "n32g031_comp.h"
#include "n32g031_exti.h"
#include "n32g031_rcc.h"
#include "n32g031_tim.h"
#include "comp.h"
#include "debug.h"

static volatile bool trigger;
static volatile uint32_t tim3_expiry;

void comp_init(void)
{
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM3 | RCC_APB1_PERIPH_COMP, ENABLE);

    TIM_TimeBaseInitType TIM_TimeBaseInitStruct;
    TIM_InitTimBaseStruct(&TIM_TimeBaseInitStruct);
    TIM_InitTimeBase(TIM3, &TIM_TimeBaseInitStruct);
    TIM_ConfigInt(TIM3, TIM_INT_UPDATE, ENABLE);

    NVIC_InitType NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    NVIC_InitStruct.NVIC_IRQChannel = COMP_IRQn;
    NVIC_Init(&NVIC_InitStruct);

    EXTI_InitType EXTI_InitStruct;
    EXTI_InitStruct.EXTI_Line = EXTI_LINE18;
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStruct);
}

void comp_prepare(uint_fast8_t probe, uint_fast8_t vref_sel)
{
    static const COMP_CTRL_INPSEL_ENUM inputs[] = { COMP_CTRL_INPSEL_PA1, COMP_CTRL_INPSEL_PA3, COMP_CTRL_INPSEL_PA7 };
    COMP_InitType COMP_InitStruct;

    COMP_StructInit(&COMP_InitStruct);
    assert(probe < sizeof(inputs) / sizeof(inputs[0]));
    COMP_InitStruct.InpSel = inputs[probe];
    COMP_InitStruct.InmSel = COMPx_CTRL_INMSEL_VREF;
    COMP_Init(&COMP_InitStruct);
    COMP_ConfigVREFx(VREF, vref_sel << 1, ENABLE);
    COMP_ConfigInt(COMP_INTEN_CMP1IEN_CFG, ENABLE);
    COMP_Enable(COMP_CTRL_EN_ENABLE);

    TIM_SetCnt(TIM3, 0);
    tim3_expiry = 0;
    trigger = false;
}

uint_fast32_t comp_start(GPIO_Module *gpio, uint_fast16_t pin, uint_fast32_t timeout)
{
    timeout >>= 16;

    __disable_irq();
    gpio->PBSC = pin;
    TIM3->CTRL1 = 1;    /* set CNTEN=1, all other bits zero */
    __enable_irq();

    while (!trigger && (tim3_expiry <= timeout))
    {
        IWDG_ReloadKey();
    }

    COMP_Enable(COMP_CTRL_EN_DISABLE);
    TIM_Enable(TIM3, DISABLE);

    return (tim3_expiry << 16) + TIM3->CNT;
}

void COMP_IRQHandler(void)
{
    TIM3->CTRL1 = 0;    /* set CNTEN=0, all other bits zero */
    asm volatile("":::"memory");
    EXTI_ClrITPendBit(EXTI_LINE18);
    trigger = true;
}

void TIM3_IRQHandler(void)
{
    tim3_expiry++;
    TIM_ClrIntPendingBit(TIM3, TIM_INT_UPDATE);
}
