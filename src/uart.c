#include <string.h>
#include "globals.h"
#include "uart.h"
#ifdef __ARM_EABI__
#include "n32g031_usart.h"

void uart_init(void)
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
    DMA_InitParam.MemAddr = (uint32_t)&uart_frame_rx;
    DMA_InitParam.BufSize = sizeof(uart_frame_rx_t);
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

    /* send version info */
    uart_frame_tx.payload[0] = 0;   /* patch */
    uart_frame_tx.payload[1] = 0;   /* minor */
    uart_frame_tx.payload[2] = 0;   /* major */
    uart_frame_tx.payload[3] = 0;   /* unknown */
    uart_send(6, 4);
}

void uart_rx_rearm(void)
{
    DMA_SetCurrDataCounter(DMA_CH5, sizeof(uart_frame_rx_t));
    DMA_EnableChannel(DMA_CH5, ENABLE);
}

void uart_send(uint_fast8_t id, size_t length)
{
    uart_frame_tx.id = id;
    uart_frame_tx.length = length;
    uart_frame_tx.counter++;
    uart_frame_tx.checksum = 0;
    for (size_t i = 0; i < length; i++)
    {
        uart_frame_tx.checksum += uart_frame_tx.payload[i];
    }
    for (size_t i = 0; i < offsetof(uart_frame_tx_t, payload) + length; i++)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXDE) == RESET) { }
        USART_SendData(USART1, uart_frame_tx.raw[i]);
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXDE) == RESET) { }
}
#endif /* __ARM_EABI__ */

void uart_send_result(void)
{
    memcpy(uart_frame_tx.payload, &result, sizeof(result));
    uart_send(2, sizeof(result));
}
