#pragma once

typedef enum : uint8_t
{
    TOOL_NONE           = 0,
    TOOL_RESISTOR       = 1,
    TOOL_INDUCTOR       = 2,
    TOOL_TEMP_DS18B20   = 3,
    TOOL_TEMP_HUM_DHT11 = 4,
    TOOL_INFRARED       = 5,
    TOOL_CALIBRATE      = 6,
} tool_t;

void tool_do(void);
