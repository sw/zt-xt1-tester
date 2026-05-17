#include <string.h>
#include "calib.h"
#include "globals.h"
#include "probe.h"
#include "tool.h"
#include "uart.h"

void tool_do(void)
{
    calib_timeout++;

    switch (tool)
    {
        case TOOL_RESISTOR:
            resistor_tool();
            break;

        case TOOL_INDUCTOR:
            inductor_tool();
            break;

        case TOOL_TEMP_DS18B20:
#ifdef __ARM_EABI__
            if (result.component == COMPONENT_DS18B20)
            {
                ds18b20_read();
            }
            else
            {
                ds18b20_detect();
            }
#endif
            break;

        case TOOL_TEMP_HUM_DHT11:
#ifdef __ARM_EABI__
            if (result.component == COMPONENT_DHT11)
            {
                dht11_read();
            }
            else
            {
                dht11_detect();
            }
#endif
            break;

        case TOOL_CALIBRATE:
            switch (calib_step)
            {
                case CALIB_PROBES_CHECK_SHORTED:
                    if (probe_all_shorted())
                    {
                        calib_step = CALIB_PROBES_RESISTANCE;
                    }
                    uart_frame_tx.payload[0] = calib_step;
                    uart_send(8, 1);
                    if (calib_timeout < 60)
                    {
                        return;
                    }
                    /* timeout expired */
                    calib_timeout = 0;
                    calib_step = CALIB_TIMEOUT;
                    break;

                case CALIB_PROBES_RESISTANCE:
                    probe_calibrate_resistance();
                    calib_step = CALIB_PROBES_CHECK_OPEN;
                    break;

                case CALIB_PROBES_CHECK_OPEN:
                    if (probe_all_open())
                    {
                        calib_step = CALIB_PROBES_CAPACITANCE;
                    }
                    break;

                case CALIB_PROBES_CAPACITANCE:
                    probe_calibrate_capacitance();
                    calib_step = CALIB_STORE;
                    break;

                case CALIB_STORE:
                    calib_step = CALIB_IDLE;
                    debug_log("Rp=%.2f\n", calibration.rp);
                    debug_log("Rd=%.2f\n", calibration.rd);
                    debug_log("Probe12_Cap=%.2f\n", calibration.probe12_cap);
                    debug_log("Probe13_Cap=%.2f\n", calibration.probe13_cap);
                    debug_log("Probe21_Cap=%.2f\n", calibration.probe21_cap);
                    debug_log("Probe23_Cap=%.2f\n", calibration.probe23_cap);
                    debug_log("Probe31_Cap=%.2f\n", calibration.probe31_cap);
                    debug_log("Probe32_Cap=%.2f\n", calibration.probe32_cap);
                    calib_write();
                    return;

                default:
                    return;
            }
            uart_frame_tx.payload[0] = calib_step;
            uart_send(8, 1);
            return;

        default:
            return;
    }

    memcpy(uart_frame_tx.payload, &result, sizeof(result));
    uart_send(4, sizeof(result));
}
