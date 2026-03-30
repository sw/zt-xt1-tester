#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ngspice/sharedspice.h>
#include "adc.h"
#include "assert.h"
#include "probes.h"
#include "spice.h"
#include "timer.h"

int send_char(char *s, int id, void *user)
{
    printf("%s:%s\n", __FUNCTION__, s);
}

int send_stat(char *stat, int id, void *user)
{
    // printf("%s:%s\n", __FUNCTION__, stat);
}

int controlled_exit(int status, NG_BOOL immediate, NG_BOOL quit, int id, void *user)
{
    if (!quit)
    {
        printf("%s status=%d immediate=%u reason=%s\n",
               __FUNCTION__, status, immediate, quit ? "quit" : "error");
    }
}

float adc_val[8];

int send_data(pvecvaluesall vec_vals, int num, int id, void *user)
{
    // printf("%s num=%d veccount=%d vecindex=%d\n", __FUNCTION__, num, vec_vals->veccount, vec_vals->vecindex);
    for (int i = 0; i < vec_vals->veccount; i++)
    {
        vecvalues *v = vec_vals->vecsa[i];
        if (strcmp(v->name, "/tp1") == 0)
        {
            printf("TP1 %fV\n", v->creal);
            adc_val[1] = v->creal;
        }
        else if (strcmp(v->name, "/tp2") == 0)
        {
            printf("TP2 %fV\n", v->creal);
            adc_val[3] = v->creal;
        }
        else if (strcmp(v->name, "/tp3") == 0)
        {
            printf("TP3 %fV\n", v->creal);
            adc_val[7] = v->creal;
        }
    }
}

int send_init_data(pvecinfoall vec_info, int id, void *user)
{
    // printf("%s\n", __FUNCTION__);
}

int bg_thread_running(NG_BOOL running, int id, void *user)
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

struct
{
    probe_mode_t direct;
    probe_mode_t r680;
    probe_mode_t r470k;
} probes[3];

unsigned int sleep_ms;
bool circ_ready;

static spice_probe_settings_t probe_settings =
{
    .rd = 15.0f,
    .rp = 15.0f,
};
static char **dut;

void spice_probe_settings_set(const spice_probe_settings_t *settings)
{
    assert(settings);
    assert(0.0f <= settings->rd);
    assert(settings->rd <= 100.0f);
    assert(0.0f <= settings->rp);
    assert(settings->rp <= 100.0f);
    probe_settings = *settings;
}

void spice_dut_set(char **s)
{
    dut = s;
    circ_ready = false;
}

void probe_configure(unsigned int probe, probe_mode_t direct, probe_mode_t r680, probe_mode_t r470k)
{
    assert(probe < 3);
    assert(direct < PROBE_MODE_NB);
    assert(r680 < PROBE_MODE_NB);
    assert(r470k < PROBE_MODE_NB);
    probes[probe].direct = direct;
    probes[probe].r680 = r680;
    probes[probe].r470k = r470k;
    circ_ready = false;
}

void msleep(unsigned int ms)
{
    sleep_ms = ms;
}

unsigned int adc_average(unsigned int channel, unsigned int num)
{
    if (!circ_ready)
    {
        char *circ_a[64];
        int i = 0;

        /* create netlist. all strings allocated on the heap and free'd after parsing */

        circ_a[i++] = strdup(".title Component Tester");

        /* supply rails */
        circ_a[i++] = strdup("v1 vcc 0 dc 5");

        /* probe resistors */
        circ_a[i++] = strdup("r11 /s1 /tp1 680");
        circ_a[i++] = strdup("r21 /s2 /tp2 680");
        circ_a[i++] = strdup("r31 /s3 /tp3 680");
        circ_a[i++] = strdup("r12 /w1 /tp1 470k");
        circ_a[i++] = strdup("r22 /w2 /tp2 470k");
        circ_a[i++] = strdup("r32 /w3 /tp3 470k");

        /* Rdson to GND */
        asprintf(&circ_a[i++], "r13 0 /an1 %f", probe_settings.rd);
        asprintf(&circ_a[i++], "r14 0 /sn1 %f", probe_settings.rd);
        asprintf(&circ_a[i++], "r15 0 /wn1 %f", probe_settings.rd);
        asprintf(&circ_a[i++], "r23 0 /an2 %f", probe_settings.rd);
        asprintf(&circ_a[i++], "r24 0 /sn2 %f", probe_settings.rd);
        asprintf(&circ_a[i++], "r25 0 /wn2 %f", probe_settings.rd);
        asprintf(&circ_a[i++], "r33 0 /an3 %f", probe_settings.rd);
        asprintf(&circ_a[i++], "r34 0 /sn3 %f", probe_settings.rd);
        asprintf(&circ_a[i++], "r35 0 /wn3 %f", probe_settings.rd);

        /* Rdson to vcc */
        asprintf(&circ_a[i++], "r16 vcc /ap1 %f", probe_settings.rp);
        asprintf(&circ_a[i++], "r17 vcc /sp1 %f", probe_settings.rp);
        asprintf(&circ_a[i++], "r18 vcc /wp1 %f", probe_settings.rp);
        asprintf(&circ_a[i++], "r26 vcc /ap2 %f", probe_settings.rp);
        asprintf(&circ_a[i++], "r27 vcc /sp2 %f", probe_settings.rp);
        asprintf(&circ_a[i++], "r28 vcc /wp2 %f", probe_settings.rp);
        asprintf(&circ_a[i++], "r36 vcc /ap3 %f", probe_settings.rp);
        asprintf(&circ_a[i++], "r37 vcc /sp3 %f", probe_settings.rp);
        asprintf(&circ_a[i++], "r38 vcc /wp3 %f", probe_settings.rp);

        /* DUT */
        for (int d = 0; dut[d]; d++)
        {
            circ_a[i++] = strdup(dut[d]);
        }

        /* dynamic probe connections */
        int v = 2;
        for (int p = 0; p < 3; p++)
        {
            if (probes[p].direct != PROBE_ANALOG)
            {
                asprintf(&circ_a[i++], "v%d /a%c%d /tp%d dc 0",
                         v++,
                         probes[p].direct == PROBE_DRV_LO ? 'n' : 'p',
                         p + 1, p + 1);
            }
            if (probes[p].r680 != PROBE_ANALOG)
            {
                asprintf(&circ_a[i++], "v%d /s%c%d /s%d dc 0",
                         v++,
                         probes[p].r680 == PROBE_DRV_LO ? 'n' : 'p',
                         p + 1, p + 1);
            }
            if (probes[p].r470k != PROBE_ANALOG)
            {
                asprintf(&circ_a[i++], "v%d /w%c%d /w%d dc 0",
                         v++,
                         probes[p].r470k == PROBE_DRV_LO ? 'n' : 'p',
                         p + 1, p + 1);
            }
        }

        circ_a[i++] = strdup(".op");
        circ_a[i++] = strdup(".end");
        circ_a[i++] = NULL;

        assert(i <= sizeof(circ_a) / sizeof(circ_a[0]));
        int e = ngSpice_Circ(circ_a);
        assert(e == 0);

        while (i--)
        {
            free(circ_a[i]);
        }

        ngSpice_Command("run");

        circ_ready = true;
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
