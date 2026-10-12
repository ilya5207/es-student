#include "pico/stdlib.h"
#include <stdio.h>
#include "led.h"
#include "log.h"
#include <string.h>
#include "device.h"
#include "profiling.h"
#include "stdio-text-protocol/stdio-text-protocol.h"
#include "api/api.h"

const uint BLINK_HALF_PERIOD_MS = 500;
uint64_t last_toggle_us = 0;

void blink(void)
{
    uint64_t now_us = time_us_64();

    if (now_us - last_toggle_us >= BLINK_HALF_PERIOD_MS * 1000)
    {
        last_toggle_us = now_us;
        led_toggle();
    }
}

int main()
{
    stdio_init_all();
    led_init();
    profiling_init();
    stdio_text_protocol_init();

    while (1)
    {
        profiling_iteration();

        blink();

       const command_t *command = stdio_text_protocol_handle();
       if (command != NULL)
       {
        api_handle(command);
       }
       
    }
}
