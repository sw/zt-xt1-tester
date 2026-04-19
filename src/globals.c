#include "globals.h"

calibration_t calibration;
uint_least8_t calib_timeout;
calib_step_t  calib_step;

uart_frame_rx_t uart_frame_rx;
uart_frame_tx_t uart_frame_tx;
volatile uint32_t uart_rx_len;
volatile bool uart_rx_pending;

tool_t tool;
uint8_t zener_enabled;

bool ir_decoded;

result_t result;

uint_fast8_t adc_sampletime;
volatile uint32_t mainloop_seconds;
volatile uint32_t mainloop_centiseconds;

volatile uint32_t tim3_cnt_comp;
volatile uint32_t tim3_expiry;
