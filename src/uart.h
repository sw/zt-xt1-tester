#pragma once

#include <stddef.h>
#include <stdint.h>

void uart_init(void);
void uart_rx_rearm(void);
void uart_send(uint_fast8_t id, size_t length);
void uart_send_result(void);
