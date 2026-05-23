#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "interface.h"

extern self_adjust_values_t self_adjust_vals;
extern uint_least8_t        self_adjust_timeout;
extern self_adjust_step_t   self_adjust_step;

extern tester_uart_frame_t uart_frame_rx;
extern tester_uart_frame_t uart_frame_tx;
extern volatile uint32_t uart_rx_len;
extern volatile bool uart_rx_pending;

extern tool_t tool;
extern uint8_t zener_enabled;

extern bool ir_decoded;

extern tester_result_t result;

extern volatile uint32_t mainloop_seconds;
extern volatile uint32_t mainloop_centiseconds;

extern volatile uint32_t tim3_cnt_comp;
extern volatile uint32_t tim3_expiry;
