#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "calib.h"
#include "component.h"
#include "globals.h"
#include "spice.h"

int test_zener(int argc, char *argv[])
{
    calib_default();
    spice_init();

    char *dut[4];
    int i = 0;
    dut[i++] = ".include \"../../../test/spice/1N5221.txt\"";
    dut[i++] = ".include \"../../../test/spice/1N4751.txt\"";
    dut[i++] = "";
    dut[i++] = NULL;
    assert(i <= sizeof(dut) / sizeof(dut[0]));

    zener_enabled = 1;

    dut[2] = "xd1 0 /tz di_1n5221b";
    spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
    memset(&result, 0xCD, sizeof(result));
    component_do_all();
    assert(result.component == COMPONENT_ZENER);
    assert(fabsf(result.diode_vf - 2.18f) < 0.01f);

    dut[2] = "xd1 0 /tz di_1n4751a";
    spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
    memset(&result, 0xCD, sizeof(result));
    component_do_all();
    assert(result.component == COMPONENT_ZENER);
    assert(fabsf(result.diode_vf - 29.8f) < 0.1f);

    spice_uninit();
    return 0;
}
