#pragma once

typedef struct
{
    float rd;
    float rp;
} spice_probe_settings_t;

void spice_init(void);
void spice_uninit(void);
void spice_probe_settings_set(const spice_probe_settings_t *settings);
void spice_dut_set(char **s);
