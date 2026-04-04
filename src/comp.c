#include "n32g031_comp.h"
#include "n32g031_exti.h"
#include "comp.h"
#include "debug.h"
#include "globals.h"

void comp_init(uint_fast8_t probe, uint_fast8_t vref_sel)
{
    static const COMP_CTRL_INPSEL_ENUM inputs[] = { COMP_CTRL_INPSEL_PA1, COMP_CTRL_INPSEL_PA3, COMP_CTRL_INPSEL_PA7 };
    COMP_InitType COMP_InitStruct;
    EXTI_InitType EXTI_InitStruct;
    NVIC_InitType NVIC_InitStruct;
    
    COMP_StructInit(&COMP_InitStruct);
    assert(probe < sizeof(inputs) / sizeof(inputs[0]));
    COMP_InitStruct.InpSel = inputs[probe];
    COMP_InitStruct.InmSel = COMPx_CTRL_INMSEL_VREF;
    COMP_Init(&COMP_InitStruct);
    COMP_Enable(ENABLE);
    COMP_ConfigVREFx(VREF, vref_sel << 1, ENABLE);
    COMP_ConfigInt(COMP_INTEN_CMP1IEN_CFG, ENABLE);

    EXTI_InitStruct.EXTI_Line = EXTI_LINE18;
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStruct);

    NVIC_InitStruct.NVIC_IRQChannel = COMP_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

uint_fast32_t comp_start(GPIO_Module *gpio, uint_fast16_t pin, uint_fast32_t timeout)
{
    tim3_cnt_comp = 0;
    tim3_expiry = 0;
    TIM_SetCnt(TIM3, 0);
    gpio->PBSC = pin;
    uint_fast32_t cnt;
    for (cnt = 0; (tim3_cnt_comp == 0) && (cnt < timeout); cnt = tim3_expiry << 16)
    {
        IWDG_ReloadKey();
    }
    return cnt + tim3_cnt_comp;
}
