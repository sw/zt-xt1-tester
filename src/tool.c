#include <string.h>
#include "component.h"
#include "globals.h"
#include "probe.h"
#include "self_adjust.h"
#include "tool.h"
#include "uart.h"

void tool_do(void)
{
    self_adjust_timeout++;

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

        case TOOL_SELF_ADJUST:
            switch (self_adjust_step)
            {
                case SELF_ADJUST_PROBES_CHECK_SHORTED:
                    if (probe_all_shorted())
                    {
                        self_adjust_step = SELF_ADJUST_PROBES_RESISTANCE;
                    }
                    ((self_adjust_state_t *)uart_frame_tx.payload)->step = self_adjust_step;
                    ((self_adjust_state_t *)uart_frame_tx.payload)->val  = self_adjust_vals;
                    uart_send(8, sizeof(self_adjust_state_t));
                    if (self_adjust_timeout < 60)
                    {
                        return;
                    }
                    /* timeout expired */
                    self_adjust_timeout = 0;
                    self_adjust_step = SELF_ADJUST_TIMEOUT;
                    break;

                case SELF_ADJUST_PROBES_RESISTANCE:
                    probe_calibrate_resistance();
                    self_adjust_step = SELF_ADJUST_PROBES_CHECK_OPEN;
                    break;

                case SELF_ADJUST_PROBES_CHECK_OPEN:
                    if (probe_all_open())
                    {
                        self_adjust_step = SELF_ADJUST_PROBES_CAPACITANCE;
                    }
                    break;

                case SELF_ADJUST_PROBES_CAPACITANCE:
                    probe_calibrate_capacitance();
                    self_adjust_step = SELF_ADJUST_STORE;
                    break;

                case SELF_ADJUST_STORE:
                    self_adjust_step = SELF_ADJUST_IDLE;
                    debug_log("Rp=%.2f\n", self_adjust_vals.rp);
                    debug_log("Rd=%.2f\n", self_adjust_vals.rd);
                    debug_log("Probe12_Cap=%.2f\n", self_adjust_vals.probe12_cap);
                    debug_log("Probe13_Cap=%.2f\n", self_adjust_vals.probe13_cap);
                    debug_log("Probe21_Cap=%.2f\n", self_adjust_vals.probe21_cap);
                    debug_log("Probe23_Cap=%.2f\n", self_adjust_vals.probe23_cap);
                    debug_log("Probe31_Cap=%.2f\n", self_adjust_vals.probe31_cap);
                    debug_log("Probe32_Cap=%.2f\n", self_adjust_vals.probe32_cap);
                    self_adjust_write();
                    return;

                default:
                    return;
            }
            ((self_adjust_state_t *)uart_frame_tx.payload)->step = self_adjust_step;
            ((self_adjust_state_t *)uart_frame_tx.payload)->val  = self_adjust_vals;
            uart_send(8, sizeof(self_adjust_state_t));
            return;

        default:
            return;
    }

    memcpy(uart_frame_tx.payload, &result, sizeof(result));
    uart_send(4, sizeof(result));
}
