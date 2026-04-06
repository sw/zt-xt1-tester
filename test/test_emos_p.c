#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "calib.h"
#include "component.h"
#include "globals.h"
#include "spice.h"

int test_emos_p(int argc, char *argv[])
{
    calib_default();
    spice_init();

    char *dut[3];
    int i = 0;
    dut[i++] = ".include \"../../../test/spice/BSS84.txt\"";
    dut[i++] = "";
    dut[i++] = NULL;
    assert(i <= sizeof(dut) / sizeof(dut[0]));

    /* sanity check: no device connected */
    spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
    component_do_all();
    assert((result.component == COMPONENT_NONE) || ((result.component == COMPONENT_CAP) && (result.capacitance_pF < 10.0f)));

    static const unsigned int probes[6][3] =
    {
        {0, 1, 2},
        {0, 2, 1},
        {1, 0, 2},
        {1, 2, 0},
        {2, 0, 1},
        {2, 1, 0},
    };

    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        asprintf(&dut[1], "xq1 /t%u /t%u /t%u bss84", probes[i][1], probes[i][0], probes[i][2]);
        spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
        component_do_all();
        assert(result.component == COMPONENT_EMOS);
        assert(result.subtype == 2);
        assert(fabsf(result.resistance - 2.23f) < 0.01f);
        assert(fabsf(result.emos_uth - 2.07f) < 0.01f);
        assert(67.0f < result.capacitance_pF);
        assert(result.capacitance_pF < 84.0f);
        assert(result.probes[0] == probes[i][0]);
        assert(result.probes[1] == probes[i][1]);
        assert(result.probes[2] == probes[i][2]);
        free(dut[1]);
    }

    spice_uninit();
    return 0;
}
