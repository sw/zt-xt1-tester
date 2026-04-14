#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include "calib.h"
#include "component.h"
#include "globals.h" /* TODO: remove */
#include "main.h"
#include "spice.h"

extern uint8_t uart_received[];
static const result_t *const result_p = (result_t *)uart_received;

static const unsigned int probes[][2] = { {0, 1}, {0, 2}, {1, 2} };

static int setup(void **state)
{
    calib_default();
    spice_init();
    return 0;
}

static int teardown(void **state)
{
    spice_uninit();
    return 0;
}

static void test_component(void **state)
{
    char *dut[2] = { NULL };
    int i = 0;

    /* TODO: 4Mohm...30Mohm - weird adjustment for large values */
    for (float r = 10.0f; r < 4e6f; r *= 75.0f)
    {
        i = (i + 1) % (sizeof(probes) / sizeof(probes[0]));

        asprintf(&dut[0], "r1 /t%u /t%u %f", probes[i][0], probes[i][1], r);
        spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
        free(dut[0]);

        expect_uint_value(uart_send, id, 2);
        expect_uint_value(uart_send, length, 88);
        component_do_all();

        assert_uint_equal(result_p->component, COMPONENT_RESISTOR);
        assert_float_equal(result_p->resistance, r, r * 0.02f + 0.5f);
        assert_true(   ((result_p->probes[0] == probes[i][0]) && (result_p->probes[2] == probes[i][1]))
                    || ((result_p->probes[0] == probes[i][1]) && (result_p->probes[2] == probes[i][0])) );
    }
}

static void test_tool(void **state)
{
    char *dut[2] = { NULL };
    int i = 0;

    /* TODO: 4Mohm...30Mohm - weird adjustment for large values */
    for (float r = 10.0f; r < 4e6f; r *= 75.0f)
    {
        i = (i + 1) % (sizeof(probes) / sizeof(probes[0]));

        asprintf(&dut[0], "r1 /t%u /t%u %f", probes[i][0], probes[i][1], r);
        spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
        free(dut[0]);

        expect_uint_value(uart_send, id, 4);
        expect_uint_value(uart_send, length, 88);
        tool = TOOL_RESISTOR;
        tool_do();

        assert_uint_equal(result_p->component, COMPONENT_RESISTOR);
        assert_float_equal(result_p->resistance, r, r * 0.02f + 0.5f);
        assert_true(   ((result_p->probes[0] == probes[i][0]) && (result_p->probes[2] == probes[i][1]))
                    || ((result_p->probes[0] == probes[i][1]) && (result_p->probes[2] == probes[i][0])) );
    }
}

int test_resistor(int argc, char *argv[])
{
    static const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test_component),
        cmocka_unit_test(test_tool),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}
