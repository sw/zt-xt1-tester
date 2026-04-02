#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "calib.h"
#include "component.h"
#include "globals.h"
#include "spice.h"

int test_diode(int argc, char *argv[])
{
    calib_default();
    spice_init();

    char *dut[5];
    int i = 0;
    dut[i++] = ".include \"../../../test/spice/1N4001.txt\"";
    dut[i++] = ".include \"../../../test/spice/1N5819.txt\"";
    dut[i++] = "";
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
        asprintf(&dut[2], "d1 /tp%u /tp%u DI_1N4001", probes[i][0] + 1, probes[i][1] + 1);
        spice_dut_set(dut);
        component_do_all();
        assert(result.component == COMPONENT_DIODE);
        assert(fabsf(result.diode_vf - 0.68f) < 0.01f);
        assert(result.diode_ir_mA < 0.0053f); /* TODO: why the variation? */
        assert(result.probes[0] == probes[i][0]);
        assert(result.probes[1] == probes[i][1]);
        free(dut[2]);
    }

    /*
        dual diodes
        result.probes aren't set, instead check the forward voltages in result.diode_vf_a
        TODO: more checks
        TODO: speed up
    */
    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        asprintf(&dut[2], "d1 /tp%u /tp%u DI_1N5819", probes[i][0] + 1, probes[i][1] + 1);

        asprintf(&dut[3], "d2 /tp%u /tp%u DI_1N4001", probes[i][0] + 1, probes[i][2] + 1);
        spice_dut_set(dut);
        component_do_all();
        assert(result.component == COMPONENT_2DIODE);
        assert(fabsf(result.diode_vf_a[i] - 0.43f) < 0.01f);
        assert(fabsf(result.diode_vf_a[(i + (i % 3 + 2)) % 6] - 5.0f) < 0.01f);
        free(dut[3]);

        asprintf(&dut[3], "d2 /tp%u /tp%u DI_1N4001", probes[i][1] + 1, probes[i][2] + 1);
        spice_dut_set(dut);
        component_do_all();
        assert(result.component == COMPONENT_2DIODE);
        assert(fabsf(result.diode_vf_a[i] - 0.43f) < 0.01f);
        assert(fabsf(result.diode_vf_a[(i + (i % 3 + 2)) % 6] - 5.0f) < 0.01f);
        free(dut[3]);

        asprintf(&dut[3], "d2 /tp%u /tp%u DI_1N4001", probes[i][2] + 1, probes[i][0] + 1);
        spice_dut_set(dut);
        component_do_all();
        assert(result.component == COMPONENT_2DIODE);
        assert(fabsf(result.diode_vf_a[i] - 0.43f) < 0.01f);
        assert(fabsf(result.diode_vf_a[(i + (i % 3 + 2)) % 6] - 5.0f) < 0.01f);
        free(dut[3]);

        asprintf(&dut[3], "d2 /tp%u /tp%u DI_1N4001", probes[i][2] + 1, probes[i][1] + 1);
        spice_dut_set(dut);
        component_do_all();
        assert(result.component == COMPONENT_2DIODE);
        assert(fabsf(result.diode_vf_a[i] - 0.43f) < 0.01f);
        assert(fabsf(result.diode_vf_a[(i + (i % 3 + 2)) % 6] - 5.0f) < 0.01f);
        free(dut[3]);

        free(dut[2]);
    }

    spice_uninit();
    return 0;
}
