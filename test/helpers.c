#include <cmocka.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "globals.h"

uint8_t uart_received[128];

void uart_rx_rearm(void) { }

void uart_send(uint_fast8_t id, size_t length)
{
    check_expected_uint(id);
    check_expected_uint(length);
    memcpy(uart_received, uart_frame_tx.payload, length);
}

float divf(float x, float y)
{
    if ((x != 0.0f) && (y != 0.0f))
    {
        return x / y;
    }
    /* the Cortex-M0 implementation returns zero instead of Inf */
    return 0.0f;
}

void mock_uart(uint_fast8_t id, uint_fast8_t counter, size_t length, void *data)
{
    uart_frame_rx.id = id;
    uart_frame_rx.counter = counter;
    memcpy(uart_frame_rx.payload, data, length);
    uart_rx_pending = true;
    uart_rx_len = offsetof(uart_frame_rx_t, payload) + length;
}

void mock_second_elapsed(void)
{
    mainloop_seconds = 1;
}

void expect_ack(void)
{
    expect_uint_value(uart_send, id, 1);
    expect_uint_value(uart_send, length, 0);
}

void expect_result(void)
{
    expect_uint_value(uart_send, id, 2);
    expect_uint_value(uart_send, length, 88);
}
