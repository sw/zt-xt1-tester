#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "component.h"
#include "globals.h"
#include "spice.h"

int test_emos_p(int argc, char *argv[])
{
    spice_init();

    bool res;
    char *dut[3];
    int i = 0;
    dut[i++] = ".include \"../../../test/spice/BSS84.txt\"";
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
        asprintf(&dut[1], "xq1 /tp%u /tp%u /tp%u BSS84", probes[i][1] + 1, probes[i][0] + 1, probes[i][2] + 1);
        spice_dut_set(dut);
        component_do_all();
        assert(result.component == COMPONENT_EMOS);
        assert(result.subtype == 2);
        assert(result.probes[0] == probes[i][0]);
        assert(result.probes[1] == probes[i][1]);
        assert(result.probes[2] == probes[i][2]);
        free(dut[1]);
    }

    spice_uninit();
    return 0;
}
