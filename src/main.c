#include "main.h"
#include <string.h>
#include "adc.h"
#include "comp.h"
#include "component.h"
#include "globals.h"
#include "helpers.h"
#include "self_adjust.h"
#include "timer.h"
#include "tool.h"
#include "uart.h"
#ifdef __ARM_EABI__
#include "n32g031_iwdg.h"
#include "n32g031_rcc.h"

static void clock_enable(void)
{
    /* enable DMA clock */
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);
    /* enable ADC clock */
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_ADC, ENABLE);
    /* enable TIM6 clock */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM6, ENABLE);
    /* enable OPAMP clock */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_OPAMP, ENABLE);
    /* enable IOPF, IOPB, IOPA clocks */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOF | RCC_APB2_PERIPH_GPIOB | RCC_APB2_PERIPH_GPIOA, ENABLE);
    /* enable AFIO clock */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO, ENABLE);
    /* enable USART1 clock */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_USART1, ENABLE);
}

static void iwdg_setup(void)
{
    IWDG_WriteConfig(IWDG_WRITE_ENABLE);
    IWDG_SetPrescalerDiv(IWDG_PRESCALER_DIV32);
    IWDG_CntReload(2500);
    IWDG_ReloadKey();
    IWDG_Enable();
}

int main(void)
{
    clock_enable();
    tim6_init();
    uart_init();
    adc_init();
    comp_init();

    /* load calibration data, initialize if not valid */
    if (!self_adjust_load())
    {
        self_adjust_default();
    }

    iwdg_setup();
    SysTick_Config(SystemCoreClock / 100);

    while (true)
    {
        main_cycle();
    }
}
#endif /* __ARM_EABI__ */

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
    self_adjust_timeout = 0;
    self_adjust_step = SELF_ADJUST_IDLE;
    if (uart_frame_rx.id == 1)
    {
        uart_send(1, 0);
        zener_enabled = uart_frame_rx.payload[0];
        component_do_all();
    }
    else if (uart_frame_rx.id == 3)
    {
        switch (uart_frame_rx.payload[0])
        {
            case TOOL_RESISTOR:
            case TOOL_INDUCTOR:
            case TOOL_TEMP_DS18B20:
            case TOOL_TEMP_HUM_DHT11:
                tool = uart_frame_rx.payload[0];
                break;
            case TOOL_INFRARED:
                if (ir_detect())
                {
                    memcpy(uart_frame_tx.payload, &result, sizeof(result));
                    uart_send(4, sizeof(result)); /* different id from uart_send_result. TODO: merge */
                }
                break;
            case TOOL_SELF_ADJUST:
                self_adjust_step = SELF_ADJUST_PROBES_CHECK_SHORTED;
                tool = uart_frame_rx.payload[0];
                break;
            default:
                tool = TOOL_NONE;
                break;
        }
    }

    uart_rx_pending = false;
    uart_rx_rearm();
no_cmd_received:
    ir_read();
    if (ir_decoded)
    {
        memcpy(uart_frame_tx.payload, &result, sizeof(result));
        uart_send(4, sizeof(result));  /* different id from uart_send_result. TODO: merge */
        ir_decoded = false;
    }
}

void main_cycle(void)
{
    iwdg_reload();
    cmd_wait_ir();
    if (mainloop_seconds)
    {
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
