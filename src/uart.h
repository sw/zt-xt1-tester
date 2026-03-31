#pragma once

#include <stddef.h>

void uart_init(void);
void uart_send(uint_fast8_t id, size_t length);
void uart_send_result(void);
