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

    char *dut[2];
    int i = 0;
    dut[i++] = "";
    dut[i++] = NULL;
    assert(i <= sizeof(dut) / sizeof(dut[0]));

    /* sanity check: no device connected */
    spice_dut_set(dut);
    component_do_all();
    assert((result.component == COMPONENT_NONE) || ((result.component == COMPONENT_CAP) && (result.capacitance_pF < 10.0f)));

    static const unsigned int probes[3][2] = { {0, 1}, {0, 2}, {1, 2} };

    /* TODO: >=100uF (cap_bat_find/cap_big) */
    for (float c = 100; c <= 100e6f; c *= 10.0f)
    {
        for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
        {
            asprintf(&dut[0], "c1 /tp%u /tp%u %fp", probes[i][0] + 1, probes[i][1] + 1, c);
            spice_dut_set(dut);
            component_do_all();
            assert(result.component == COMPONENT_CAP);
            assert(fabsf(result.capacitance_pF - c) < c * 0.013f + 10.0f);
            assert(   ((result.probes[0] == probes[i][0]) && (result.probes[2] == probes[i][1]))
                   || ((result.probes[0] == probes[i][1]) && (result.probes[2] == probes[i][0])) );
            free(dut[0]);
        }
    }

    spice_uninit();
    return 0;
}
