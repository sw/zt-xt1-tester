#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "calib.h"
#include "interface.h"

extern calibration_t calibration;
extern uint_least8_t calib_timeout;
extern calib_step_t  calib_step;

extern tester_uart_frame_t uart_frame_rx;
extern tester_uart_frame_t uart_frame_tx;
extern volatile uint32_t uart_rx_len;
extern volatile bool uart_rx_pending;

extern tool_t tool;
extern uint8_t zener_enabled;

extern bool ir_decoded;

extern tester_result_t result;

extern uint_fast8_t adc_sampletime;
extern volatile uint32_t mainloop_seconds;
extern volatile uint32_t mainloop_centiseconds;

extern volatile uint32_t tim3_cnt_comp;
extern volatile uint32_t tim3_expiry;
