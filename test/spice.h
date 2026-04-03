#pragma once

typedef struct
{
    float rd;
    float rp;
    float probe12_cap;
    float probe13_cap;
    float probe21_cap;
    float probe23_cap;
    float probe31_cap;
    float probe32_cap;
} spice_probe_settings_t;

void spice_init(void);
void spice_uninit(void);
void spice_probe_settings_set(const spice_probe_settings_t *settings);
void spice_dut_set(char **s);
