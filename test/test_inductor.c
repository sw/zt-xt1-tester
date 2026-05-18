#include <cmocka.h>
#include <stdlib.h>
#include "calib.h"
#include "helpers.h"
#include "interface.h"
#include "main.h"
#include "spice.h"

extern uint8_t uart_received[];
static const tester_result_t *const result_p = (tester_result_t *)uart_received;

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
    char *dut[3] = { NULL };
    int i = 0;

    for (float l = 10e3f; l <= 100e3f; l *= 10.0f)
    {
        i = (i + 1) % (sizeof(probes) / sizeof(probes[0]));

        asprintf(&dut[0], "l1 /t%u /xx %fu", probes[i][0], l);
        /* simulate 2ohm resistance */
        asprintf(&dut[1], "r1 /xx /t%u 2", probes[i][1]);
        spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
        free(dut[0]);
        free(dut[1]);

        expect_ack();
        expect_result();
        mock_uart(1, 0, 1, (uint8_t[]){0});
        main_cycle();

        assert_uint_equal(result_p->component, COMPONENT_INDUCTOR);
        assert_float_equal(result_p->inductance_uH, l, l * 0.02f);
        assert_float_equal(result_p->resistance, 2.0f, 0.1f);
        assert_true(   ((result_p->probes[0] == probes[i][0]) && (result_p->probes[2] == probes[i][1]))
                    || ((result_p->probes[0] == probes[i][1]) && (result_p->probes[2] == probes[i][0])) );
    }
}

static void test_tool(void **state)
{
    char *dut[3] = { NULL };
    int i = 0;

    for (float l = 10e3f; l <= 100e3f; l *= 10.0f)
    {
        i = (i + 1) % (sizeof(probes) / sizeof(probes[0]));

        asprintf(&dut[0], "l1 /t%u /xx %fu", probes[i][0], l);
        /* simulate 2ohm resistance */
        asprintf(&dut[1], "r1 /xx /t%u 2", probes[i][1]);
        spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
        free(dut[0]);
        free(dut[1]);

        mock_uart(3, 0, 1, (uint8_t[]){TOOL_INDUCTOR});
        main_cycle();
        expect_uint_value(uart_send, id, 4);
        expect_uint_value(uart_send, length, 88);
        mock_second_elapsed();
        main_cycle();

        assert_uint_equal(result_p->component, COMPONENT_INDUCTOR);
        assert_float_equal(result_p->inductance_uH, l, l * 0.02f);
        assert_float_equal(result_p->resistance, 2.0f, 0.1f);
        assert_true(   ((result_p->probes[0] == probes[i][0]) && (result_p->probes[2] == probes[i][1]))
                    || ((result_p->probes[0] == probes[i][1]) && (result_p->probes[2] == probes[i][0])) );
    }
}

int test_inductor(int argc, char *argv[])
{
    static const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test_component),
        cmocka_unit_test(test_tool),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}
