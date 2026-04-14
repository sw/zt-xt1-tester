#include <cmocka.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "globals.h"

uint8_t uart_received[128];

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
