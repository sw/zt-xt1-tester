#include "component.h"
#include "globals.h"
#include "uart.h"

void component_do_all(void)
{
    result.component = COMPONENT_NONE;
    if (!(zener() || jfet() || dmos() || thy_triac() || bjt() || emos() || igbt() || ujt() || diode()))
    {
        cap_bat();
        resistor();
        inductor();
    }
    uart_send_result();
}
