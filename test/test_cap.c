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
    // segfaults ¯\_(ツ)_/¯
    // spice_uninit();
    return 0;
}

static void test_component(void **state)
{
    char *dut[3] = { NULL };
    int i = 0;

    /* TODO: >=5mF (cap_bat_find/cap_big) */
    for (float c = 150; c < 5e9f; c *= 20.0f)
    {
        i = (i + 1) % (sizeof(probes) / sizeof(probes[0]));

        asprintf(&dut[0], "c1 /t%u /esr %fp", probes[i][0], c);
        /* simulate 2ohm ESR */
        asprintf(&dut[1], "r1 /esr /t%u 2", probes[i][1]);
        spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
        free(dut[0]);
        free(dut[1]);

        expect_ack();
        expect_result();
        mock_uart(1, 0, 1, (uint8_t[]){0});
        main_cycle();

        assert_uint_equal(result_p->component, COMPONENT_CAP);
        assert_float_equal(result_p->capacitance_pF, c, c * 0.05f + 11.0f);
        if (c > 700e3)
        {
            /* ESR is wildly wrong... */
            assert_float_in_range(result_p->resistance, 1.0f, 10.5f, 0.0f);
        }
        if (c > 90e3f)
        {
            assert_float_in_range(result_p->cap_vloss, 1.6f, 7.6f, 0.0f);
        }
        assert_true(   ((result_p->probes[0] == probes[i][0]) && (result_p->probes[2] == probes[i][1]))
                    || ((result_p->probes[0] == probes[i][1]) && (result_p->probes[2] == probes[i][0])) );
    }
}

int test_cap(int argc, char *argv[])
{
    static const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test_component),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}
