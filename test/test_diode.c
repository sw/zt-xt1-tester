#include <cmocka.h>
#include <stdlib.h>
#include "calib.h"
#include "helpers.h"
#include "interface.h"
#include "main.h"
#include "spice.h"

extern uint8_t uart_received[];
static const tester_result_t *const result_p = (tester_result_t *)uart_received;

static const unsigned int probes[][3] =
{
    {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}
};

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

static void test_single(void **state)
{
    char *dut[3] = { ".include \"../../../test/spice/1N4001.txt\"" };

    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        asprintf(&dut[1], "d1 /t%u /t%u di_1n4001", probes[i][0], probes[i][2]);
        spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
        free(dut[1]);

        expect_ack();
        expect_result();
        mock_uart(1, 0, 1, (uint8_t[]){0});
        main_cycle();

        assert_uint_equal(result_p->component, COMPONENT_DIODE);
        assert_float_equal(result_p->diode_vf, 0.68f, 0.01f);

        /* TODO: original firmware has no delay in diode_forward_reverse, measures higher reverse current */
        assert_float_in_range(result_p->current_mA, 0.0f, 0.1f, 0.0f);
        assert_float_in_range(result_p->capacitance_pF, 20.0f, 33.0f, 0.0f);

        assert_uint_equal(result_p->probes[0], probes[i][0]);
        assert_uint_equal(result_p->probes[2], probes[i][2]);
    }
}

/*
    dual diodes
    result.probes aren't set, instead check the forward voltages in result.diode_vf_a
    TODO: more checks
    TODO: speed up
*/
static void test_dual(void **state)
{
    char *dut[5] = {
        ".include \"../../../test/spice/1N4001.txt\"",
        ".include \"../../../test/spice/1N5819.txt\""
    };

    for (int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        asprintf(&dut[2], "d1 /t%u /t%u di_1n5819", probes[i][0], probes[i][1]);

        asprintf(&dut[3], "d2 /t%u /t%u di_1n4001", probes[i][0], probes[i][2]);
        spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
        free(dut[3]);
        expect_ack();
        expect_result();
        mock_uart(1, 0, 1, (uint8_t[]){0});
        main_cycle();
        assert_uint_equal(result_p->component, COMPONENT_2DIODE);
        assert_float_equal(result_p->diode_vf_a[i], 0.43f, 0.01f);
        assert_float_equal(result_p->diode_vf_a[(i + (i % 3 + 2)) % 6], 5.0f, 0.01f);

        asprintf(&dut[3], "d2 /t%u /t%u di_1n4001", probes[i][1], probes[i][2]);
        spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
        free(dut[3]);
        expect_ack();
        expect_result();
        mock_uart(1, 0, 1, (uint8_t[]){0});
        main_cycle();
        assert_uint_equal(result_p->component, COMPONENT_2DIODE);
        assert_float_equal(result_p->diode_vf_a[i], 0.43f, 0.01f);
        assert_float_equal(result_p->diode_vf_a[(i + (i % 3 + 2)) % 6], 5.0f, 0.01f);

        asprintf(&dut[3], "d2 /t%u /t%u di_1n4001", probes[i][2], probes[i][0]);
        spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
        free(dut[3]);
        expect_ack();
        expect_result();
        mock_uart(1, 0, 1, (uint8_t[]){0});
        main_cycle();
        assert_uint_equal(result_p->component, COMPONENT_2DIODE);
        assert_float_equal(result_p->diode_vf_a[i], 0.43f, 0.01f);
        assert_float_equal(result_p->diode_vf_a[(i + (i % 3 + 2)) % 6], 5.0f, 0.01f);

        asprintf(&dut[3], "d2 /t%u /t%u di_1n4001", probes[i][2], probes[i][1]);
        spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
        free(dut[3]);
        expect_ack();
        expect_result();
        mock_uart(1, 0, 1, (uint8_t[]){0});
        main_cycle();
        assert_uint_equal(result_p->component, COMPONENT_2DIODE);
        assert_float_equal(result_p->diode_vf_a[i], 0.43f, 0.01f);
        assert_float_equal(result_p->diode_vf_a[(i + (i % 3 + 2)) % 6], 5.0f, 0.01f);

        free(dut[2]);
    }
}

int test_diode(int argc, char *argv[])
{
    static const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test_single),
        cmocka_unit_test(test_dual),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}
