#include "tool.h"

extern tool_t tool;

void tool_do(void)
{
    if (tool == TOOL_RESISTOR)
    {
        //test_resistor_tool();
    }
    else if (tool == TOOL_INDUCTOR)
    {
        //test_inductor_tool();
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

    }
}
