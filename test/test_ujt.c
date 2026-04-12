#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "calib.h"
#include "component.h"
#include "globals.h"
#include "spice.h"

int test_ujt(int argc, char *argv[])
{
    calib_default();
    spice_init();

    char *dut[3];
    int i = 0;
    dut[i++] = ".include \"../../../test/spice/2N2646.txt\"";
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
        /*                       E   B1   B2 */
        asprintf(&dut[1], "xq1 /t%u /t%u /t%u X2N2646", probes[i][0], probes[i][1], probes[i][2]);
        spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
        free(dut[1]);

        memset(&result, 0xCD, sizeof(result));
        component_do_all();
        assert(result.component == COMPONENT_UJT);
        assert(fabsf(result.bjt_ube - 0.87f) < 0.01f);
        assert(fabsf(result.resistance - 6137.0f) < 1.0f);
        assert(result.probes[0] == probes[i][0]);
        assert(result.probes[2] == probes[i][1]);
        assert(result.probes[1] == probes[i][2]);
    }

    spice_uninit();
    return 0;
}
