#include "globals.h"

calibration_t calibration;

uart_frame_rx_t uart_frame_rx;
uart_frame_tx_t uart_frame_tx;
volatile uint32_t uart_rx_len;
volatile bool uart_rx_pending;

tool_t tool;

result_t result;

volatile uint32_t mainloop_seconds;
volatile uint32_t mainloop_centiseconds;
