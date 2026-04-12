#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "calib.h"
#include "component.h"
#include "globals.h"
#include "spice.h"

int test_darlington_pnp(int argc, char *argv[])
{
    calib_default();
    spice_init();

    char *dut[4];
    int i = 0;
    dut[i++] = ".include \"../../../test/spice/2N3906.txt\"";
    dut[i++] = "";
    dut[i++] = "";
    dut[i++] = NULL;
    assert(i <= sizeof(dut) / sizeof(dut[0]));

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
        /* for now join two 2N3906s. TODO: find workable model of a true darlington */
        asprintf(&dut[1], "q1 /t%u /xx /t%u 2n3906", probes[i][1], probes[i][2]);
        asprintf(&dut[2], "q2 /t%u /t%u /xx 2n3906", probes[i][1], probes[i][0]);
        spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
        free(dut[1]);
        free(dut[2]);

        memset(&result, 0xCD, sizeof(result));
        component_do_all();
        assert(result.component == COMPONENT_DARLINGTON);
        assert(result.subtype == 2);
        assert(fabsf(result.hfe - 74.9f) < 1.0f);
        assert(fabsf(result.bjt_ube - 1.67f) < 0.01f);
        assert(result.probes[0] == probes[i][0]);
        assert(result.probes[1] == probes[i][1]);
        assert(result.probes[2] == probes[i][2]);
    }

    spice_uninit();
    return 0;
}
