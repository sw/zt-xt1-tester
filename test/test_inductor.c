#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "calib.h"
#include "component.h"
#include "globals.h"
#include "spice.h"

int test_inductor(int argc, char *argv[])
{
    calib_default();
    spice_init();

    char *dut[3];
    int i = 0;
    dut[i++] = "";
    dut[i++] = "";
    dut[i++] = NULL;
    assert(i <= sizeof(dut) / sizeof(dut[0]));

    /* sanity check: no device connected */
    spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
    component_do_all();
    assert((result.component == COMPONENT_NONE) || ((result.component == COMPONENT_CAP) && (result.capacitance_pF < 10.0f)));

    static const unsigned int probes[3][2] = { {0, 1}, {0, 2}, {1, 2} };

    for (float l = 10e3f; l <= 100e3f; l *= 10.0f)
    {
        for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
        {
            asprintf(&dut[0], "l1 /t%u /xx %fu", probes[i][0], l);
            /* simulate 2ohm resistance */
            asprintf(&dut[1], "r1 /xx /t%u 2", probes[i][1]);
            spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
            free(dut[0]);
            free(dut[1]);

            component_do_all();

            assert(result.component == COMPONENT_INDUCTOR);
            assert(fabsf(result.inductance_uH - l) < l * 0.02f);
            assert(fabsf(result.resistance - 2) < 2 * 0.05f);
            assert(   ((result.probes[0] == probes[i][0]) && (result.probes[2] == probes[i][1]))
                   || ((result.probes[0] == probes[i][1]) && (result.probes[2] == probes[i][0])) );
        }
    }

    spice_uninit();
    return 0;
}
