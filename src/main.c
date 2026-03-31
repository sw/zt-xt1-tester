#include <stdio.h>
#include <stdint.h>
#include "n32g031_iwdg.h"
#include "n32g031_rcc.h"
#include "n32g031_tim.h"
#include "adc.h"
#include "calib.h"
#include "globals.h"
#include "tool.h"
#include "uart.h"

static void clock_enable(void)
{
    /* enable DMA clock */
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);
    /* enable ADC clock */
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_ADC, ENABLE);
    /* enable TIM3, TIM6 clocks */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM3 | RCC_APB1_PERIPH_TIM6, ENABLE);
    /* enable OPAMP, COMP, COMPFILT clocks */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_OPAMP | RCC_APB1_PERIPH_COMPFILT | RCC_APB1_PERIPH_COMP, ENABLE);
    /* enable IOPF, IOPB, IOPA clocks */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOF | RCC_APB2_PERIPH_GPIOB | RCC_APB2_PERIPH_GPIOA, ENABLE);
    /* enable AFIO clock */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO, ENABLE);
    /* enable TIM1, TIM8, USART1 clocks */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_USART1 | RCC_APB2_PERIPH_TIM8 | RCC_APB2_PERIPH_TIM1, ENABLE);
}

static void tim3_init(void)
{
    TIM_TimeBaseInitType TIM_TimeBaseInitStruct;
    NVIC_InitType NVIC_InitStruct;
    
    TIM_TimeBaseInitStruct.Prescaler = 0;
    TIM_TimeBaseInitStruct.CntMode = TIM_CNT_MODE_UP;
    TIM_TimeBaseInitStruct.Period = 0xffff;
    TIM_TimeBaseInitStruct.ClkDiv = TIM_CLK_DIV1;
    TIM_TimeBaseInitStruct.CapCh3FromCompEn = true;
    /* TODO: some fields not initialized??? */
    TIM_InitTimeBase(TIM3, &TIM_TimeBaseInitStruct);
    TIM_ConfigInt(TIM3, TIM_INT_UPDATE, ENABLE);
    TIM_Enable(TIM3, ENABLE);

    NVIC_InitStruct.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd = 1;
    NVIC_Init(&NVIC_InitStruct);
}

static void tim6_init(void)
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

static void iwdg_setup(void)
{
    IWDG_WriteConfig(IWDG_WRITE_ENABLE);
    IWDG_SetPrescalerDiv(IWDG_PRESCALER_DIV32);
    IWDG_CntReload(2500);
    IWDG_ReloadKey();
    IWDG_Enable();
    /* TODO: write disable? */
}

/* wait for command received via UART, meanwhile decode infrared */
static void cmd_wait_ir(void)
{
    if (!uart_rx_pending)
    {
        goto no_cmd_received;
    }
    uart_frame_tx.counter = uart_frame_rx.counter;
no_cmd_received:
}

int main(void)
{
    clock_enable();
    tim6_init();
    uart_init();
    adc_init();
    tim3_init();

    /* load calibration data, initialize if not valid */
    if (calib_load())
    {
        calib_default();
        calib_write();
    }

    iwdg_setup();
    SysTick_Config(SystemCoreClock / 100);

    while (true)
    {
        do
        {
            IWDG_ReloadKey();
            cmd_wait_ir();
        } while (mainloop_seconds == 0);
        mainloop_seconds = 0;
        tool_do();
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(const uint8_t *expr, const uint8_t *file, uint32_t line)
{
    while (1)
    {
    }
}
#endif /* USE_FULL_ASSERT */
