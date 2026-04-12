#include "component.h"
#include "globals.h"
#include "uart.h"

void component_do_all(void)
{
    result.component = COMPONENT_NONE;
    adc_sampletime = 5; /* ADC_SAMP_TIME_42CYCLES5 */
    if (!(zener() || jfet() || dmos() || thy_triac() || darlington() || bjt() || emos() || igbt() || ujt() || diode()))
    {
        cap_bat();
        resistor();
        inductor();
    }
    uart_send_result();
}
