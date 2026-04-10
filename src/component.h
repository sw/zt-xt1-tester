#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum : uint8_t
{
    COMPONENT_NONE       =  0,
    COMPONENT_JFET       =  2,
    COMPONENT_BJT        =  4,
    COMPONENT_DARLINGTON =  5,
    COMPONENT_EMOS       =  7,
    COMPONENT_DIODE      = 11,
    COMPONENT_2DIODE     = 12,
    COMPONENT_BATTERY    = 13,
    COMPONENT_CAP        = 14,
    COMPONENT_RESISTOR   = 15,
    COMPONENT_INDUCTOR   = 16,
    COMPONENT_ZENER      = 17,
} component_t;

void component_do_all(void);

bool bjt(void);

bool cap_bat(void);
bool cap_small(unsigned int p0, unsigned int p1, unsigned int p2, bool subtract_probe);

bool darlington(void);

bool diode(void);
void diode_forward_reverse(unsigned int pa, unsigned int pk);

bool emos(void);

bool inductor(void);
bool inductor_tool(void);

bool jfet(void);

bool resistor(void);
void resistor_measure(int a, int b, int param);
bool resistor_tool(void);

bool zener(void);
