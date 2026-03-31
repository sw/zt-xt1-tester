#pragma once

#include <assert.h>
#include <stdint.h>
#include "tool.h"

typedef struct
{
    uint8_t id;
    uint8_t counter;
    uint8_t unused[4];  /* length / checksum ? */
    uint8_t test_type;
    uint8_t unknown[127];
} uart_frame_rx_t;
static_assert(sizeof(uart_frame_rx_t) == 134);

typedef union
{
    struct
    {
        uint8_t id;
        uint8_t counter;
        uint16_t length;
        uint16_t checksum;
        uint8_t payload[88];
    };
    uint8_t raw[94];
} uart_frame_tx_t;
static_assert(sizeof(uart_frame_tx_t) == 94);

extern uart_frame_rx_t uart_frame_rx;
extern uart_frame_tx_t uart_frame_tx;
extern tool_t tool;
extern float calib_rp;
extern float calib_rd;
extern float result_hfe;
extern float result_ic;
extern float result_ube;
extern unsigned int result_subtype;
extern unsigned int result_probes[3];
