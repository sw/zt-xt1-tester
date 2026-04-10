#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "n32g031_iwdg.h"
#include "n32g031_rcc.h"
#include "n32g031_tim.h"
#include "adc.h"
#include "calib.h"
#include "globals.h"
#include "timer.h"
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

static void iwdg_setup(void)
{
    IWDG_WriteConfig(IWDG_WRITE_ENABLE);
    IWDG_SetPrescalerDiv(IWDG_PRESCALER_DIV32);
    IWDG_CntReload(2500);
    IWDG_ReloadKey();
    IWDG_Enable();
}

/* wait for command received via UART, meanwhile decode infrared */
static void cmd_wait_ir(void)
{
    if (!uart_rx_pending)
    {
        goto no_cmd_received;
    }
    uart_frame_tx.counter = uart_frame_rx.counter;
    result.component = COMPONENT_NONE;
    mainloop_centiseconds = 0;
    mainloop_seconds = 0;
    tool = TOOL_NONE;
    calib_timeout = 0;
    calib_request = 0;
    if (uart_frame_rx.id == 1)
    {
        uart_send(1, 0);
        zener_enabled = uart_frame_rx.test_type;
        component_do_all();
    }
    else if (uart_frame_rx.id == 3)
    {
        switch (uart_frame_rx.test_type)
        {
            case TOOL_RESISTOR:
            case TOOL_INDUCTOR:
            case TOOL_TEMP_DS18B20:
            case TOOL_TEMP_HUM_DHT11:
                tool = uart_frame_rx.test_type;
                break;
            case TOOL_INFRARED:
                // infrared_detected = infrared_detect();
                if (false) // (infrared_detected)
                {
                    memcpy(uart_frame_tx.payload, &result, sizeof(result));
                    uart_send(4, sizeof(result)); /* different id from uart_send_result. TODO: merge */
                }
                break;
            case TOOL_CALIBRATE:
                calib_request = true;
                tool = uart_frame_rx.test_type;
                break;
        }
    }

    uart_rx_pending = false;
    /* re-arm DMA transfer. TODO: move to uart.c */
    DMA_SetCurrDataCounter(DMA_CH5, sizeof(uart_frame_rx_t));
    DMA_EnableChannel(DMA_CH5, ENABLE);
no_cmd_received:
    // infrared_read();
    if (false) // (infrared_decoded)
    {
        memcpy(uart_frame_tx.payload, &result, sizeof(result));
        uart_send(4, sizeof(result));  /* different id from uart_send_result. TODO: merge */
        // infrared_decoded = false;
    }
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
