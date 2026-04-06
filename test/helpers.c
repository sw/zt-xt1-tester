void uart_send_result(void) { }

float divf(float x, float y)
{
    if ((x != 0.0f) && (y != 0.0f))
    {
        return x / y;
    }
    /* the Cortex-M0 implementation returns zero instead of Inf */
    return 0.0f;
}
