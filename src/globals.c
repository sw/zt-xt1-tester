#include "globals.h"

self_adjust_values_t self_adjust_vals;
uint_least8_t        self_adjust_timeout;
self_adjust_step_t   self_adjust_step;

tester_uart_frame_t uart_frame_rx;
tester_uart_frame_t uart_frame_tx;
volatile uint32_t uart_rx_len;
volatile bool uart_rx_pending;

tool_t tool;
uint8_t zener_enabled;

bool ir_decoded;

tester_result_t result;

volatile uint32_t mainloop_seconds;
volatile uint32_t mainloop_centiseconds;

volatile uint32_t tim3_cnt_comp;
volatile uint32_t tim3_expiry;
