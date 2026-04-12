#include <assert.h>
#include <string.h>
#include "calib.h"
#include "component.h"
#include "globals.h"
#include "spice.h"

int test_none(int argc, char *argv[])
{
    calib_default();
    spice_init();

    char *dut[1] = { NULL };

    spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
    memset(&result, 0xCD, sizeof(result));
    component_do_all();
    assert((result.component == COMPONENT_NONE) || ((result.component == COMPONENT_CAP) && (result.capacitance_pF < 10.0f)));

    spice_uninit();
    return 0;
}
