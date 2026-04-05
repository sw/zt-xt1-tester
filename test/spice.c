#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ngspice/sharedspice.h>
#include "adc.h"
#include "probe.h"
#include "spice.h"
#include "timer.h"

static int send_char(char *s, int id, void *user)
{
    assert((strncmp(s, "stderr Warning:", strlen("stderr Warning:")) != 0)
        || (strncmp(s, "stderr Warning: losing old state for circuit", strlen("stderr Warning: losing old state for circuit")) == 0) );
    //puts(s);
}

static int send_stat(char *stat, int id, void *user)
{
    // printf("%s:%s\n", __FUNCTION__, stat);
}

static int controlled_exit(int status, NG_BOOL immediate, NG_BOOL quit, int id, void *user)
{
    if (!quit)
    {
        printf("%s status=%d immediate=%u reason=%s\n",
               __FUNCTION__, status, immediate, quit ? "quit" : "error");
    }
}

static float adc_val[8];
static unsigned int comp_probe;
static char comp_probe_s[4 + 1];
static float comp_threshold;
static uint_fast8_t comp_vref_sel;
static uint_fast32_t comp_cnt;
static unsigned int comp_step;
static unsigned int comp_pullup;

static int send_data(pvecvaluesall vec_vals, int num, int id, void *user)
{
    // printf("%s num=%d veccount=%d vecindex=%d\n", __FUNCTION__, num, vec_vals->veccount, vec_vals->vecindex);
    for (int i = 0; i < vec_vals->veccount; i++)
    {
        vecvalues *v = vec_vals->vecsa[i];
        if (comp_step)
        {
            if (strcmp(v->name, comp_probe_s) == 0)
            {
                comp_cnt += comp_step;
            }
        }
        else if (strcmp(v->name, "/ad1") == 0)
        {
            //printf("AD1 %.3fV\n", v->creal);
            adc_val[1] = v->creal;
        }
        else if (strcmp(v->name, "/ad2") == 0)
        {
            //printf("AD2 %.3fV\n", v->creal);
            adc_val[3] = v->creal;
        }
        else if (strcmp(v->name, "/ad3") == 0)
        {
            //printf("AD3 %.3fV\n", v->creal);
            adc_val[7] = v->creal;
        }
    }
}

static int send_init_data(pvecinfoall vec_info, int id, void *user)
{
    // printf("%s\n", __FUNCTION__);
}

static int bg_thread_running(NG_BOOL running, int id, void *user)
{
    printf("%s:%u\n", __FUNCTION__, running);
}

void spice_init(void)
{
    ngSpice_Init(send_char, send_stat, controlled_exit, send_data, send_init_data, bg_thread_running, NULL);
}

void spice_uninit(void)
{
    ngSpice_Command("quit");
}

static struct
{
    probe_mode_t direct;
    probe_mode_t r680;
    probe_mode_t r470k;
} probes[3];

static uint_fast32_t sleep_ms;
static bool op_ready;

static spice_probe_settings_t probe_settings =
{
    .rd = 15.0f,
    .rp = 15.0f,
    .probe12_cap = 26.28f,
    .probe13_cap = 31.97f,
    .probe21_cap = 28.04f,
    .probe23_cap = 31.97f,
    .probe31_cap = 27.93f,
    .probe32_cap = 26.70f,
};
static char **dut;

void spice_probe_settings_set(const spice_probe_settings_t *settings)
{
    assert(settings);
    assert(0.0f <= settings->rd);
    assert(settings->rd <= 100.0f);
    assert(0.0f <= settings->rp);
    assert(settings->rp <= 100.0f);
    assert(0.0f <= settings->probe12_cap);
    assert(settings->probe12_cap <= 100.0f);
    assert(0.0f <= settings->probe13_cap);
    assert(settings->probe13_cap <= 100.0f);
    assert(0.0f <= settings->probe21_cap);
    assert(settings->probe21_cap <= 100.0f);
    assert(0.0f <= settings->probe23_cap);
    assert(settings->probe23_cap <= 100.0f);
    assert(0.0f <= settings->probe31_cap);
    assert(settings->probe31_cap <= 100.0f);
    assert(0.0f <= settings->probe32_cap);
    assert(settings->probe32_cap <= 100.0f);
    probe_settings = *settings;
}

void spice_dut_set(char **s)
{
    dut = s;
    op_ready = false;
}

void probe_configure(uint_fast8_t probe, probe_mode_t direct, probe_mode_t r680, probe_mode_t r470k)
{
    assert(probe < 3);
    assert(direct < PROBE_MODE_NB);
    assert(r680 < PROBE_MODE_NB);
    assert(r470k < PROBE_MODE_NB);
    probes[probe].direct = direct;
    probes[probe].r680 = r680;
    probes[probe].r470k = r470k;
    op_ready = false;
}

void tim6_msleep(uint_fast32_t ms)
{
    sleep_ms = ms;
}

static void spice_prepare(const char *analysis, ...)
{
    char *circ_a[64];
    int i = 0;

    /* create netlist. all strings allocated on the heap and free'd after parsing */

    circ_a[i++] = strdup(".title Component Tester");

    /* supply rails */
    circ_a[i++] = strdup("v1 +5V 0 dc 5");

    /* probe resistors */
    circ_a[i++] = strdup("r11 /s1 /tp1 680");
    circ_a[i++] = strdup("r21 /s2 /tp2 680");
    circ_a[i++] = strdup("r31 /s3 /tp3 680");
    circ_a[i++] = strdup("r12 /w1 /tp1 470k");
    circ_a[i++] = strdup("r22 /w2 /tp2 470k");
    circ_a[i++] = strdup("r32 /w3 /tp3 470k");

    /* ADC input resistances */
    circ_a[i++] = strdup("r10 /ad1 /tp1 1500");
    circ_a[i++] = strdup("r20 /ad2 /tp2 1500");
    circ_a[i++] = strdup("r30 /ad3 /tp3 1500");
    /* ADC sample and hold capacitors */
    circ_a[i++] = strdup("c10 0 /ad1 13p");
    circ_a[i++] = strdup("c20 0 /ad2 13p");
    circ_a[i++] = strdup("c30 0 /ad3 13p");

    /* GPIO MOSFETs */
    /*
        Other possibly relevant parameters:
            FC - Body diode coefficient for forward-bias depletion capacitance formula
            A  - Non-linear Cgd capacitance parameter
    */
    /* TODO: figure out how to use probe_settings.probeXX_cap */
    asprintf(&circ_a[i++], ".model NMOS VDMOS NCHAN "
        "cgdmax=2p "
        "cgdmin=2p "
        "cgs=2p "
        "cjo=2p "
        "rd=%.0f "
        "vto=+1.5"
        , probe_settings.rd);
    asprintf(&circ_a[i++], ".model PMOS VDMOS PCHAN "
        "cgdmax=2p "
        "cgdmin=2p "
        "cgs=2p "
        "cjo=2p "
        "rd=%.0f "
        "vto=-1.5"
        , probe_settings.rp);

    /* TODO: tame this mess */
    /* use a pulsed voltage source to drive the PMOS gates so the initial condition will have a different value */
    circ_a[i++] = strdup("v2 /pg 0 pulse(5 0)");

    if (comp_step && (comp_probe == 0) && (comp_pullup == 0))
    {
        circ_a[i++] = strdup("MQ11 /TP1 /pg  0  NMOS");
        circ_a[i++] = strdup("MQ12 /TP1 /pg +5V PMOS");
    }
    else
    {
        asprintf(&circ_a[i++], "MQ11 /TP1 %s  0  NMOS", probes[0].direct == PROBE_DRV_LO ? "+5V" : " 0 ");
        asprintf(&circ_a[i++], "MQ12 /TP1 %s +5V PMOS", probes[0].direct == PROBE_DRV_HI ? " 0 " : "+5V");
    }

    if (comp_step && (comp_probe == 0) && (comp_pullup == 1))
    {
        circ_a[i++] = strdup("MQ13 /S1 /pg  0  NMOS");
        circ_a[i++] = strdup("MQ14 /S1 /pg +5V PMOS");
    }
    else
    {
        asprintf(&circ_a[i++], "MQ13 /S1 %s  0  NMOS", probes[0].r680 == PROBE_DRV_LO ? "+5V" : " 0 ");
        asprintf(&circ_a[i++], "MQ14 /S1 %s +5V PMOS", probes[0].r680 == PROBE_DRV_HI ? " 0 " : "+5V");
    }

    if (comp_step && (comp_probe == 0) && (comp_pullup == 2))
    {
        circ_a[i++] = strdup("MQ15 /W1 /pg  0  NMOS");
        circ_a[i++] = strdup("MQ16 /W1 /pg +5V PMOS");
    }
    else
    {
        asprintf(&circ_a[i++], "MQ15 /W1 %s  0  NMOS", probes[0].r470k == PROBE_DRV_LO ? "+5V" : " 0 ");
        asprintf(&circ_a[i++], "MQ16 /W1 %s +5V PMOS", probes[0].r470k == PROBE_DRV_HI ? " 0 " : "+5V");
    }


    if (comp_step && (comp_probe == 1) && (comp_pullup == 0))
    {
        circ_a[i++] = strdup("MQ21 /TP2 /pg  0  NMOS");
        circ_a[i++] = strdup("MQ22 /TP2 /pg +5V PMOS");
    }
    else
    {
        asprintf(&circ_a[i++], "MQ21 /TP2 %s  0  NMOS", probes[1].direct == PROBE_DRV_LO ? "+5V" : " 0 ");
        asprintf(&circ_a[i++], "MQ22 /TP2 %s +5V PMOS", probes[1].direct == PROBE_DRV_HI ? " 0 " : "+5V");
    }
    
    if (comp_step && (comp_probe == 1) && (comp_pullup == 1))
    {
        circ_a[i++] = strdup("MQ23 /S2 /pg  0  NMOS");
        circ_a[i++] = strdup("MQ24 /S2 /pg +5V PMOS");
    }
    else
    {
        asprintf(&circ_a[i++], "MQ23 /S2 %s  0  NMOS", probes[1].r680 == PROBE_DRV_LO ? "+5V" : " 0 ");
        asprintf(&circ_a[i++], "MQ24 /S2 %s +5V PMOS", probes[1].r680 == PROBE_DRV_HI ? " 0 " : "+5V");
    }

    if (comp_step && (comp_probe == 1) && (comp_pullup == 2))
    {
        circ_a[i++] = strdup("MQ25 /W2 /pg  0  NMOS");
        circ_a[i++] = strdup("MQ26 /W2 /pg +5V PMOS");
    }
    else
    {
        asprintf(&circ_a[i++], "MQ25 /W2 %s  0  NMOS", probes[1].r470k == PROBE_DRV_LO ? "+5V" : " 0 ");
        asprintf(&circ_a[i++], "MQ26 /W2 %s +5V PMOS", probes[1].r470k == PROBE_DRV_HI ? " 0 " : "+5V");
    }

    
    if (comp_step && (comp_probe == 2) && (comp_pullup == 0))
    {
        circ_a[i++] = strdup("MQ31 /TP3 /pg  0  NMOS");
        circ_a[i++] = strdup("MQ32 /TP3 /pg +5V PMOS");
    }
    else
    {
        asprintf(&circ_a[i++], "MQ31 /TP3 %s  0  NMOS", probes[2].direct == PROBE_DRV_LO ? "+5V" : " 0 ");
        asprintf(&circ_a[i++], "MQ32 /TP3 %s +5V PMOS", probes[2].direct == PROBE_DRV_HI ? " 0 " : "+5V");
    }

    if (comp_step && (comp_probe == 2) && (comp_pullup == 1))
    {
        circ_a[i++] = strdup("MQ33 /S3 /pg  0  NMOS");
        circ_a[i++] = strdup("MQ34 /S3 /pg +5V PMOS");
    }
    else
    {
        asprintf(&circ_a[i++], "MQ33 /S3 %s  0  NMOS", probes[2].r680 == PROBE_DRV_LO ? "+5V" : " 0 ");
        asprintf(&circ_a[i++], "MQ34 /S3 %s +5V PMOS", probes[2].r680 == PROBE_DRV_HI ? " 0 " : "+5V");
    }

    if (comp_step && (comp_probe == 2) && (comp_pullup == 2))
    {
        circ_a[i++] = strdup("MQ35 /W3 /pg  0  NMOS");
        circ_a[i++] = strdup("MQ36 /W3 /pg +5V PMOS");
    }
    else
    {
        asprintf(&circ_a[i++], "MQ35 /W3 %s  0  NMOS", probes[2].r470k == PROBE_DRV_LO ? "+5V" : " 0 ");
        asprintf(&circ_a[i++], "MQ36 /W3 %s +5V PMOS", probes[2].r470k == PROBE_DRV_HI ? " 0 " : "+5V");
    }

    /* DUT */
    for (int d = 0; dut[d]; d++)
    {
        circ_a[i++] = strdup(dut[d]);
    }

    va_list args;
    va_start(args, analysis);
    vasprintf(&circ_a[i++], analysis, args);
    va_end(args);

    circ_a[i++] = strdup(".end");
    circ_a[i++] = NULL;

    assert(i <= sizeof(circ_a) / sizeof(circ_a[0]));
    int e = ngSpice_Circ(circ_a);
    assert(e == 0);

    while (i--)
    {
        free(circ_a[i]);
    }
}

uint_fast16_t adc_average(uint_fast8_t channel, uint_fast16_t num)
{
    if (!op_ready)
    {
        spice_prepare(".op");
        ngSpice_Command("run");
        op_ready = true;
    }
    int val = adc_val[channel] * (4095.0f / 5.0f);
    if (val < 0)
    {
        val = 0;
    }
    if (val > 4095)
    {
        val = 4095;
    }
    return val;
}

void comp_init(uint_fast8_t probe, uint_fast8_t vref_sel)
{
    assert(probe < 3);
    comp_probe = probe;
    snprintf(comp_probe_s, sizeof(comp_probe_s), "/ad%u", probe + 1);
    assert(vref_sel < 64);
    comp_threshold = vref_sel * (5.0f / 63.0f); /* TODO: 63 or 64? */
}

uint_fast32_t comp_start(unsigned int unused, unsigned int pullup, uint_fast32_t timeout)
{
    assert(pullup < 3);

    op_ready = false;
    comp_pullup = pullup;

    char *stop;
    asprintf(&stop, "stop when v(%s) > %f", comp_probe_s, comp_threshold);

    /* speed up by doing coarse simulation step first */
    uint_fast32_t limit;
    for (comp_step = 100, limit = timeout / 100;
         comp_step;
         comp_step /= 10, limit /= 10)
    {
        comp_cnt = 0;
        /* TODO: investigate uic */
        spice_prepare(".tran %fns %fms", comp_step / (48e6 / 1e9), timeout / (48e6 / 1e3));
        ngSpice_Command(stop);
        ngSpice_Command("run");

        if (comp_cnt > limit)
        {
            break;
        }
    }

    free(stop);
    comp_step = 0;
    return comp_cnt;
}
