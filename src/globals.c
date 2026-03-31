#include "globals.h"

uart_frame_rx_t uart_frame_rx;
uart_frame_tx_t uart_frame_tx;
tool_t tool;
float calib_rp = 15;
float calib_rd = 15;
float result_hfe;
float result_ic;
float result_ube;
unsigned int result_subtype;
unsigned int result_probes[3];
