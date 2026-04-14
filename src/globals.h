#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "calib.h"
#include "component.h"
#include "debug.h"
#include "main.h"
#include "tool.h"

typedef struct
{
    uint8_t id;
    uint8_t counter;
    uint8_t unused[4];  /* length / checksum ? */
    uint8_t payload[128];
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

extern calibration_t calibration;
extern uint_least8_t calib_timeout;
extern calib_step_t  calib_step;

extern uart_frame_rx_t uart_frame_rx;
extern uart_frame_tx_t uart_frame_tx;
extern volatile uint32_t uart_rx_len;
extern volatile bool uart_rx_pending;

extern tool_t tool;
extern uint8_t zener_enabled;

extern result_t result;

extern uint_fast8_t adc_sampletime;
extern volatile uint32_t mainloop_seconds;
extern volatile uint32_t mainloop_centiseconds;

extern volatile uint32_t tim3_cnt_comp;
extern volatile uint32_t tim3_expiry;
