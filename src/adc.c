#include "n32g031_adc.h"
#include "n32g031_opamp.h"
#include "debug.h"
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
    do {
        flag = ADC_GetFlagStatusNew(ADC, ADC_FLAG_RDY);
    } while (flag == RESET);
    do {
        flag = ADC_GetFlagStatusNew(ADC, ADC_FLAG_PD_RDY);
    } while (flag != RESET);
    do {
        flag = ADC_GetFlagStatusNew(ADC, ADC_FLAG_VREF_RDY);
    } while (flag == RESET);
    ADC_EnableSoftwareStartConv(ADC, ENABLE);

    OPAMP_InitStruct.Mod = OPAMP_CS_FOLLOW;
    OPAMP_InitStruct.Gain = OPAMP_CS_PGA_GAIN_2;
    OPAMP_InitStruct.TimeAutoMuxEn = DISABLE;
    OPAMP_Init(&OPAMP_InitStruct);
    OPAMP_SetVpSel(OPAMP_CS_VPSEL_PF0);

    gpio_init(GPIOF, GPIO_PIN_0, GPIO_MODE_ANALOG, GPIO_NO_PULL);
}

uint_fast16_t adc_single(uint_fast8_t channel)
{
    #define CTRL2_EXT_TRIG_SWSTART_SET   ((uint32_t)0x00500000)

    /* original firmware sets SAMPT2 to zero, but that is for channels 8 to 15 */
    ADC->SAMPT3 = 0;
    ADC->RSEQ3 = channel;

    /* erratum 5.2? throw away first conversion result */
    ADC->STS = 0;
    ADC->CTRL2 |= CTRL2_EXT_TRIG_SWSTART_SET;
    while (!(ADC->STS & ADC_FLAG_ENDC)) { }

    ADC->STS = 0;
    ADC->CTRL2 |= CTRL2_EXT_TRIG_SWSTART_SET;
    while (!(ADC->STS & ADC_FLAG_ENDC)) { }

    return ADC->DAT;
}

uint_fast16_t adc_average(uint_fast8_t channel, uint_fast16_t num)
{
    uint_fast16_t i;
    uint_fast32_t sum = 0;
    assert(num < UINT_FAST32_MAX / 0xFFF);

    ADC_ConfigRegularChannel(ADC, channel, 1, ADC_SAMP_TIME_600CYCLES5);

    /* erratum 5.2? throw away first conversion result */
    ADC_EnableSoftwareStartConv(ADC, ENABLE);
    while (ADC_GetFlagStatus(ADC, ADC_FLAG_ENDC) == RESET) {}
    ADC_ClearFlag(ADC, ADC_FLAG_ENDC);
    ADC_ClearFlag(ADC, ADC_FLAG_STR);

    for (i = 0; i < num; i++)
    {
        ADC_EnableSoftwareStartConv(ADC, ENABLE);
        while (ADC_GetFlagStatus(ADC, ADC_FLAG_ENDC) == RESET) {}
        ADC_ClearFlag(ADC, ADC_FLAG_ENDC);
        ADC_ClearFlag(ADC, ADC_FLAG_STR);

        sum += ADC_GetDat(ADC);
    }
    return sum / num;
}
