#include <stdio.h>
#include <stdint.h>
#include "n32g031_rcc.h"
#include "n32g031_tim.h"
#include "n32g031_usart.h"

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

static void uart_init(void)
{
    USART_InitType USART_InitStruct;
    NVIC_InitType NVIC_InitStruct;
    DMA_InitType DMA_InitParam;
    GPIO_InitType GPIO_InitStruct;
    
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF4_USART1;
    GPIO_InitStruct.GPIO_Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.GPIO_Pull = GPIO_PULL_UP;
    GPIO_InitStruct.GPIO_Current = GPIO_DC_HIGH;
    GPIO_InitPeripheral(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitPeripheral(GPIOA, &GPIO_InitStruct);

    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.WordLength = USART_WL_8B;
    USART_InitStruct.StopBits = USART_STPB_1;
    USART_InitStruct.Parity = USART_PE_NO;
    USART_InitStruct.HardwareFlowControl = USART_HFCTRL_NONE;
    USART_InitStruct.Mode = USART_MODE_RX | USART_MODE_TX;
    USART_Init(USART1, &USART_InitStruct);

    USART_Enable(USART1, ENABLE);
    USART_EnableDMA(USART1, USART_DMAREQ_RX, ENABLE);
    USART_ConfigInt(USART1, USART_INT_IDLEF, ENABLE);

    NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
    
    DMA_DeInit(DMA_CH5);
    DMA_StructInit(&DMA_InitParam);
    DMA_InitParam.PeriphAddr = USART1->DAT;
    //DMA_InitParam.MemAddr = (uint)PTR_in_frm_id_080091f8;
    DMA_InitParam.BufSize = 0x86;
    DMA_InitParam.DMA_MemoryInc = DMA_MEM_INC_ENABLE;
    DMA_InitParam.PeriphInc = DMA_PERIPH_INC_DISABLE;
    DMA_InitParam.Direction = DMA_DIR_PERIPH_SRC;
    DMA_InitParam.Priority = DMA_PRIORITY_VERY_HIGH;
    DMA_InitParam.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitParam.MemDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitParam.CircularMode = DMA_MODE_NORMAL;
    DMA_InitParam.Mem2Mem = DMA_M2M_DISABLE;
    DMA_Init(DMA_CH5, &DMA_InitParam);

    DMA_RequestRemap(DMA_REMAP_USART1_RX, DMA, DMA_CH5, ENABLE);
    DMA_EnableChannel(DMA_CH5, ENABLE);

    /*
    src = PTR_DAT_080091fc;
    log(s_Version:%x_08009200,*(undefined4 *)(PTR_DAT_080091fc + 0xc));
    dst = PTR_in_frm_id_080091f8;
    PTR_in_frm_id_080091f8[-0x80] = (char)*(undefined4 *)(src + 0xc);
    dst[-0x7f] = (char)((uint)*(undefined4 *)(src + 0xc) >> 8);
    dst[-0x7e] = (char)((uint)*(undefined4 *)(src + 0xc) >> 0x10);
    dst[-0x7d] = (char)((uint)*(undefined4 *)(src + 0xc) >> 0x18);
    usart_send_frame(6,4);
    */
}

int main(void)
{
    clock_enable();
    tim6_init();
    uart_init();
}

#ifdef USE_FULL_ASSERT
void assert_failed(const uint8_t *expr, const uint8_t *file, uint32_t line)
{
    while (1)
    {
    }
}
#endif /* USE_FULL_ASSERT */
