#include "adc.h"
#include "diode.h"
#include "globals.h"
#include "probe.h"
#include "timer.h"

void diode_forward_reverse(unsigned int pa, unsigned int pk)
{
    static const unsigned int channels[3] = {1, 3, 7};

    probe_configure(pa, PROBE_ANALOG, PROBE_DRV_HI, PROBE_ANALOG);
    probe_configure(pk, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    float ua = adc_average(channels[pa], 1000) * (5.0f / 4095.0f);
    float uk = adc_average(channels[pk], 1000) * (5.0f / 4095.0f);
    result.diode_vf = ua - uk;

    probe_configure(pa, PROBE_DRV_LO, PROBE_ANALOG, PROBE_ANALOG);
    probe_configure(pk, PROBE_ANALOG, PROBE_DRV_HI, PROBE_DRV_HI);
    tim6_msleep(1);
    probe_configure(pk, PROBE_ANALOG, PROBE_ANALOG, PROBE_DRV_HI);
    uk = adc_average(channels[pk], 1000) * (5.0f / 4095.0f);
    result.diode_ir_mA = (5.0f - uk) / 470.0f;
}
