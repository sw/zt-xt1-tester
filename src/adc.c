#include "n32g031_adc.h"
#include "n32g031_opamp.h"
#include "gpio.h"

void adc_init(void)
{
    ADC_InitType ADC_InitStruct;
    OPAMP_InitType OPAMP_InitStruct;
    FlagStatus flag;
    
    /* timing clock must be 1MHz */
    RCC_ConfigAdc1mClk(RCC_ADC1MCLK_SRC_HSI, RCC_ADC1MCLK_DIV8);
    /* set sampling clock to AHB_CLK / 6 = 8MHz */
    ADC_ConfigClk(ADC_CTRL3_CKMOD_AHB, RCC_ADCHCLK_DIV6);
    ADC_InitStruct.MultiChEn = DISABLE;
    ADC_InitStruct.ContinueConvEn = DISABLE;
    ADC_InitStruct.ExtTrigSelect = ADC_EXT_TRIGCONV_NONE;
    ADC_InitStruct.DatAlign = ADC_DAT_ALIGN_R;
    ADC_InitStruct.ChsNumber = 1;
    ADC_Init(ADC, &ADC_InitStruct);
    ADC_EnableVrefint(ENABLE);
    ADC_Enable(ADC, ENABLE);
    /* TODO: ADC_GetFlagStatus_diff ??? */
    do {
        flag = ADC_GetFlagStatus(ADC, ADC_FLAG_ENDC_ANY);
    } while (flag == RESET);
    do {
        flag = ADC_GetFlagStatus(ADC, ADC_FLAG_JENDC_ANY);
    } while (flag != RESET);
    do {
        flag = ADC_GetFlagStatus(ADC, ADC_FLAG_JENDC);
    } while (flag == RESET);
    ADC_EnableSoftwareStartConv(ADC, ENABLE);

    OPAMP_InitStruct.Mod = OPAMP_CS_FOLLOW;
    OPAMP_InitStruct.Gain = OPAMP_CS_PGA_GAIN_2;
    OPAMP_InitStruct.TimeAutoMuxEn = DISABLE;
    OPAMP_Init(&OPAMP_InitStruct);
    OPAMP_SetVpSel(OPAMP_CS_VPSEL_PF0);

    gpio_init(GPIOF, GPIO_PIN_0, GPIO_MODE_ANALOG, GPIO_NO_PULL);
}
