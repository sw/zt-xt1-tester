#include "bjt.h"
#include "darlington.h"
#include "cap.h"
#include "diode.h"
#include "emos.h"
#include "globals.h"
#include "inductor.h"
#include "resistor.h"
#include "uart.h"

static bool zener(void) { return false; }
static bool jfet(void) { return false; }
static bool dmos(void) { return false; }
static bool thy_triac(void) { return false; }
static bool igbt(void) { return false; }
static bool ujt(void) { return false; }

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
