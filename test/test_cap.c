#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "calib.h"
#include "component.h"
#include "globals.h"
#include "spice.h"

int test_cap(int argc, char *argv[])
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

    /* TODO: >=5mF (cap_bat_find/cap_big) */
    for (float c = 100; c < 5e9f; c *= 20.0f)
    {
        for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
        {
            asprintf(&dut[0], "c1 /t%u /esr %fp", probes[i][0], c);
            /* simulate 2ohm ESR */
            asprintf(&dut[1], "r1 /esr /t%u 2", probes[i][1]);
            spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
            free(dut[0]);
            free(dut[1]);

            component_do_all();

            assert(result.component == COMPONENT_CAP);
            assert(fabsf(result.capacitance_pF - c) < c * 0.05f + 11.0f);
            /* ESR is wildly wrong... */
            assert((c < 700e3f) || ((result.resistance > 1.0f) && (result.resistance < 10.5f)));
            assert((c < 90e3f) || ((result.cap_vloss > 1.6f) && (result.cap_vloss < 7.6f)));
            assert(   ((result.probes[0] == probes[i][0]) && (result.probes[2] == probes[i][1]))
                   || ((result.probes[0] == probes[i][1]) && (result.probes[2] == probes[i][0])) );
        }
    }

    // segfaults
    // spice_uninit();
    return 0;
}
