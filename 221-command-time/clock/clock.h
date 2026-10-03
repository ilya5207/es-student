#pragma once

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"


#define CLOCKS_FCO_SRC_VALUE_ROSC_CLKSRC _u(0x03)
#define CLOCKS_FCO_SRC_VALUE_CLK_REF _u(0x08)
#define CLOCKS_FCO_SRC_VALUE_CLK_SYS _u(0x09)
#define CLOCKS_FCO_SRC_VALUE_CLK_PERI _u(0x0a)
#define CLOCKS_FCO_SRC_VALUE_CLK_USB _u(0x0b)
#define CLOCKS_FCO_SRC_VALUE_CLK_ADC _u(0x0c)

void clk_info (void);