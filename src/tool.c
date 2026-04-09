#include <string.h>
#include "globals.h"
#include "inductor.h"
#include "resistor.h"
#include "uart.h"
#include "tool.h"

void tool_do(void)
{
    if (tool == TOOL_RESISTOR)
    {
        resistor_tool();
    }
    else if (tool == TOOL_INDUCTOR)
    {
        inductor_tool();
    }
    else if (tool == TOOL_TEMP_DS18B20)
    {
        /*
        if (PTR_config_ram_08009490[0x28] == COMPONENT_TEMPERATURE) {
            read_temperature();
        }
        else {
            read_temperature_humidity();
        }
        */
    }
    else if (tool == TOOL_TEMP_HUM_DHT11)
    {
        /*
        if (PTR_config_ram_08009490[0x28] == COMPONENT_DHT11) {
            read_dht11();
        }
        else {
            check_dht11();
        }
        */
    }
    else if (tool == TOOL_CALIBRATE)
    {

        uart_send(8, 1);
        return;
    }

    memcpy(uart_frame_tx.payload, &result, sizeof(result));
    uart_send(4, sizeof(result));
}
