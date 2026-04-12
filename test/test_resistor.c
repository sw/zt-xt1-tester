#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "calib.h"
#include "component.h"
#include "globals.h"
#include "spice.h"
#include "tool.h"

int test_resistor(int argc, char *argv[])
{
    calib_default();
    spice_init();

    char *dut[2];
    int i = 0;
    dut[i++] = "";
    dut[i++] = NULL;
    assert(i <= sizeof(dut) / sizeof(dut[0]));

    /* sanity check: no device connected */
    spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
    memset(&result, 0xCD, sizeof(result));
    tool = TOOL_RESISTOR;
    tool_do();
    assert(result.component == COMPONENT_NONE);

    static const unsigned int probes[3][2] = { {0, 1}, {0, 2}, {1, 2} };

    /* TODO: 4Mohm...30Mohm - weird adjustment for large values */
    for (float r = 10.0f; r < 4e6f; r *= 75.0f)
    {
        i = (i + 1) % (sizeof(probes) / sizeof(probes[0]));

        asprintf(&dut[0], "r1 /t%u /t%u %f", probes[i][0], probes[i][1], r);
        spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
        free(dut[0]);

        memset(&result, 0xCD, sizeof(result));
        component_do_all();
        assert(result.component == COMPONENT_RESISTOR);
        assert(fabsf(result.resistance - r) < r * 0.02f + 0.5f);
        assert(   ((result.probes[0] == probes[i][0]) && (result.probes[2] == probes[i][1]))
                || ((result.probes[0] == probes[i][1]) && (result.probes[2] == probes[i][0])) );

        memset(&result, 0xCD, sizeof(result));
        tool = TOOL_RESISTOR;
        tool_do();
        assert(result.component == COMPONENT_RESISTOR);
        assert(fabsf(result.resistance - r) < r * 0.02f + 0.5f);
        assert(   ((result.probes[0] == probes[i][0]) && (result.probes[2] == probes[i][1]))
                || ((result.probes[0] == probes[i][1]) && (result.probes[2] == probes[i][0])) );
    }

    spice_uninit();
    return 0;
}
