#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "component.h"
#include "globals.h"
#include "spice.h"

int test_bjt_npn(int argc, char *argv[])
{
    calibration.rp = 15.0f;
    calibration.rd = 15.0f;

    spice_init();

    bool res;
    char *dut[3];
    int i = 0;
    dut[i++] = ".include \"../../../test/spice/2N3904.txt\"";
    dut[i++] = "";
    dut[i++] = NULL;
    assert(i <= sizeof(dut) / sizeof(dut[0]));

    /* sanity check: no device connected */
    spice_dut_set(dut);
    component_do_all();
    assert(result.component == COMPONENT_NONE);

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
        asprintf(&dut[1], "q1 /tp%u /tp%u /tp%u 2N3904 temp=27", probes[i][1] + 1, probes[i][0] + 1, probes[i][2] + 1);
        spice_dut_set(dut);
        component_do_all();
        assert(result.component == COMPONENT_BJT);
        assert(result.subtype == 1);
        assert(fabsf(result.hfe - 152.6f) < 0.1f);
        assert(fabsf(result.ube - 0.68f) < 0.01f);
        assert(result.probes[0] == probes[i][0]);
        assert(result.probes[1] == probes[i][1]);
        assert(result.probes[2] == probes[i][2]);
        free(dut[1]);
    }

    spice_uninit();
    return 0;
}
