#include "bjt.h"
#include "globals.h"
#include "uart.h"

static bool zener(void) { return false; }
static bool jfet(void) { return false; }
static bool dmos(void) { return false; }
static bool thy_triac(void) { return false; }
static bool darlington(void) { return false; }
static bool emos(void) { return false; }
static bool igbt(void) { return false; }
static bool ujt(void) { return false; }
static bool diode(void) { return false; }
static void cap_bat(void) { }
static void resistor(void) { }
static bool inductor(void) { return false; }

void component_do_all(void)
{
    result.component = COMPONENT_NONE;
    adc_sampletime = 5;
    if (!(zener() || jfet() || dmos() || thy_triac() || darlington() || bjt() || emos() || igbt() || ujt() || diode()))
    {
        cap_bat();
        resistor();
        inductor();
    }
    uart_send_result();
}
