#include "adc.h"
#include "component.h"
#include "globals.h"
#include "opamp.h"
#include "probe.h"
#include "timer.h"

bool zener(void)
{
    if (zener_enabled == 0)
    {
        return false;
    }
    probe_configure(0, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(1, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(2, PROBE_ANALOG, PROBE_ANALOG, PROBE_ANALOG);
    opamp_enable(1);
#ifndef __ARM_EABI__
    tim6_usleep(100); /* not in original firmware, required for simulation */
#endif
    result.diode_vf = adc_average(6, 100) * (39.0f / 4095.0f);
    opamp_enable(0);
    if (result.diode_vf < 5.0f)
    {
        // ???
        result.diode_vf += 0.1f;
    }
    if (result.diode_vf < 34.0f)
    {
        result.component = COMPONENT_ZENER;
        debug_log("Zener %.2fV\n", result.diode_vf);
        return true;
    }
    return false;
}
