#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bjt.h"
#include "spice.h"

extern float result_hfe;
extern float result_ic;
extern float result_ube;
extern unsigned int result_probes[3];
extern unsigned int result_subtype;

int test_bjt_pnp(int argc, char *argv[])
{
    spice_init();

    bool res;
    char *dut[3];
    int i = 0;
    dut[i++] = ".include \"../../../test/spice/2N3906.txt\"";
    dut[i++] = "";
    dut[i++] = NULL;
    assert(i <= sizeof(dut) / sizeof(dut[0]));

    /* sanity check: no device connected */
    spice_dut_set(dut);
    res = bjt();
    assert(!res);

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
        asprintf(&dut[1], "q1 /tp%u /tp%u /tp%u 2N3906 temp=27", probes[i][1] + 1, probes[i][0] + 1, probes[i][2] + 1);
        spice_dut_set(dut);
        res = bjt();
        fflush(stdout);
        assert(res);
        assert(result_subtype == 2);
        assert(fabsf(result_hfe - 205.0f) < 0.1f);
        assert(fabsf(result_ube - 0.72f) < 0.01f);
        assert(result_probes[0] == probes[i][0]);
        assert(result_probes[1] == probes[i][1]);
        assert(result_probes[2] == probes[i][2]);
        free(dut[1]);
    }

    spice_uninit();
    return 0;
}
