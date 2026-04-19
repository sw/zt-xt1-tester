#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum : uint8_t
{
    COMPONENT_NONE       =  0,
    COMPONENT_JFET       =  2,
    COMPONENT_DMOS       =  3,
    COMPONENT_BJT        =  4,
    COMPONENT_DARLINGTON =  5,
    COMPONENT_UJT        =  6,
    COMPONENT_EMOS       =  7,
    COMPONENT_IGBT       =  8,
    COMPONENT_THYRISTOR  =  9,
    COMPONENT_TRIAC      = 10,
    COMPONENT_DIODE      = 11,
    COMPONENT_2DIODE     = 12,
    COMPONENT_BATTERY    = 13,
    COMPONENT_CAP        = 14,
    COMPONENT_RESISTOR   = 15,
    COMPONENT_INDUCTOR   = 16,
    COMPONENT_ZENER      = 17,
    COMPONENT_INFRARED   = 18,
    COMPONENT_DS18B20    = 19,
    COMPONENT_DHT11      = 20,
} component_t;

void component_do_all(void);

bool bjt(void);

bool cap_bat(void);
bool cap_small(unsigned int p0, unsigned int p1, unsigned int p2, bool subtract_probe);

bool darlington(void);

bool dht11_detect(void);
bool dht11_read(void);

bool diode(void);
void diode_forward_reverse(unsigned int pa, unsigned int pk);

bool dmos(void);

bool ds18b20_detect(void);
bool ds18b20_read(void);

bool emos(void);

bool igbt(void);

bool inductor(void);
bool inductor_tool(void);

bool ir_detect(void);
void ir_read(void);

bool jfet(void);

bool resistor(void);
void resistor_measure(int a, int b, int param);
bool resistor_tool(void);

bool thy_triac(void);

bool ujt(void);

bool zener(void);
