#include "api/api-commands.h"

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "device/device.h"
#include "profiling/profiling.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "firmware.h"
#include "systime/systime.h"
#include "edge-task/edge-task.h"
#include "uart-task/uart-task.h"

#define CALC_PI_DEFAULT_TERMS 1000000u

static const size_t TEXT_MAX_LENGTH = 32;

static void cmd_info(const command_t *command)
{
        (void)command;
        device_info_t info;
        device_get_info(&info);
        printf("name: %s\n", FIRMWARE_NAME);
        printf("version: %s\n", FIRMWARE_VERSION);    
        printf("project: %s\n", FIRMWARE_PROJECT);
        printf("repo: %s\n", FIRMWARE_REPO);
        printf("board: %s\n", info.board);
        printf("serial: %s\n", info.serial);
        printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n", (unsigned)info.manufacturer, (unsigned)info.part, (unsigned)info.revision);
        printf("pico-sdk: %s\n", info.sdk_version);    
}

static void cmd_uptime(const command_t *command)
{
    (void)command;
    printf("uptime: %llu ms\n", (unsigned long long)systime_ms());
}

static bool parse_u32(const char *text, uint32_t *result)
{
    if (text == NULL || *text == '\0' || *text == '-')
    {
        return false;
    }

    errno = 0;
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 10);

    if (errno == ERANGE || *end != '\0' || value > UINT32_MAX)
    {
        return false;
    }

    *result = (uint32_t)value;
    return true;
}

static void cmd_pi_start(const command_t *command)
{
    uint32_t terms = CALC_PI_DEFAULT_TERMS;
    if (command->argc > 1 || (command->argc == 1 && !parse_u32(command->argv[0], &terms))) 
    {
        printf("error: usage pi_start [terms]\n");
        return;
    }

    if (terms == 0)
    {
        printf("error: terms must be greater than 0\n");    
        return;
    }

    if (!pi_task_start(terms))
    {
        printf("error: pi is busy\n");
        return;
    }
    printf("pi: started, %u terms\n", (unsigned)terms);    
}

static void cmd_pi(const command_t *command)
{
    (void)command;
    pi_task_result_t result;
    pi_task_get_result(&result);

    switch (result.state)
    {
    case PI_TASK_NOT_STARTED:
        printf("pi: not started\n");
        break;
    case PI_TASK_RUNNING:
        printf("pi: running\n");
        break;
    case PI_TASK_READY:
        printf("pi: %.9f in %llu ms\n", result.value, (unsigned long long)result.time_ms);
        break;   
    }    
}

static void cmd_profiling(const command_t *command)
{
        (void)command;
#if PROFILING_ENABLED        
        for (uint32_t id = 0; id < PROFILING_STOPWATCH_COUNT; id++)
        {
            profiling_result_t result;
            if (profiling_get(id, &result))
            {
                printf("%-10s count %-10u mean %.2f us max %u us\n", result.name, (unsigned)result.count, result.mean_us, (unsigned)result.max_us);
            }
        }
    #else
        printf("profiling is disabled in this build\n");
    #endif           
}

static void cmd_profiling_reset(const command_t *command)
{
    (void)command;
    profiling_reset();
    printf("profiling reset\n");
}

static const char *led_state_name(led_state_t state)
{
    switch (state)
    {
    case LED_STATE_OFF:
        return "off";
    case LED_STATE_ON:
        return "on";
    case LED_STATE_BLINK:
        return "blink";
    default:
        return "unknown";
    }
}

static void print_led(void)
{
    printf("led: %s, period %u ms\n", led_state_name(led_task_get_state()), (unsigned)led_task_get_period_ms());
}

static void cmd_led_enable(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_ON);
    print_led();
}

static void cmd_led_disable(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_OFF);
    print_led();
}

static void cmd_led_blink(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_BLINK);
    print_led();
}

static void cmd_led_period(const command_t *command)
{
    uint32_t period_ms;
    if (command->argc != 1 || !parse_u32(command->argv[0], &period_ms))
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }
    if (!led_task_set_period_ms(period_ms))
    {
        printf("error: period_ms must be greater than 0\n");
        return;
    }
    print_led();
}

static void cmd_button(const command_t *command)
{
    (void)command;
    printf("button: %s, presses %u\n", button_task_is_pressed() ? "pressed" : "released", (unsigned)button_task_get_press_count());
}

static void print_edge(platform_line_mode_t mode, bool ok, uint32_t rise_us, uint32_t fall_us)
{
    const char *name = mode == PLATFORM_LINE_PUSH_PULL ? "pp" : "od";

    if (!ok)
    {
        printf("edge %s: timeout\n", name);
        return;
    }

    printf("edge %s: rise %.1f ms, fall %.1f ms\n", name, rise_us / 1000.0, fall_us / 1000.0);
}

static void cmd_edge(const command_t *command)
{
    if (command->argc != 1 || (strcmp(command->argv[0], "pp") != 0 && strcmp(command->argv[0], "od") != 0))
    {
        printf("error: usage edge pp|od\n");
        return;
    }

    platform_line_mode_t mode = strcmp(command->argv[0], "pp") == 0 ? PLATFORM_LINE_PUSH_PULL : PLATFORM_LINE_OPEN_DRAIN;
    if ( !edge_task_start(mode, print_edge))
    {
        printf("error: edge is busy\n");
    }
}

static void print_bytes(const uint8_t *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        putchar(isprint((unsigned char)data[i]) ? data[i] : '.');
    }
}

static void print_errors(uint32_t errors)
{
    if (errors == 0)
    {
        printf("err: none");
        return;
    }

    printf("err:");
    if (errors & PLATFORM_UART_FE)
    {
        printf("FE");
    }
    if (errors & PLATFORM_UART_PE)
    {
        printf("PE");
    }
    if (errors & PLATFORM_UART_BE)
    {
        printf("BE");
    }
    if (errors & PLATFORM_UART_OE)
    {
        printf("OE");
    }
}

static void cmd_loop(const command_t *command)
{
    if (command->argc != 1 || (strcmp(command->argv[0], "internal") != 0 && strcmp(command->argv[0], "pins") != 0))
    {
        printf("error: usage loop internal|pins\n");
        return;
    }

    platform_uart_loop_set(strcmp(command->argv[0], "internal") == 0);
    printf("loop: %s\n", command->argv[0]);
}

static void print_echo(const uint8_t *data, size_t size, uint32_t errors)
{
    printf("rx: ");
    print_bytes(data, size);
    putchar(' ');
    print_errors(errors);
    putchar('\n');
}

static bool text_argument(const command_t *command, const uint8_t **data, size_t *size)
{
    if (command->argc != 1)
    {
        printf("error: usage %s <text>\n", command->name);
        return false; 
    }

    *size = strlen(command->argv[0]);
    if (*size > TEXT_MAX_LENGTH)
    {
        printf("error: text longer than 32\n");
        return false;
    }

    *data = (const uint8_t *)command->argv[0];
    return true;
}

static void cmd_echo(const command_t *command)
{
    const uint8_t *data;
    size_t size;
    if (!text_argument(command, &data, &size))
    {
        return;
    }

    if (!uart_task_start(PLATFORM_UART0, PLATFORM_UART0, data, size, print_echo))
    {
        printf("error: uart is busy\n");
    }
}

const api_command_t api_commands[] = {
    {"info", "device passport", cmd_info},
    {"uptime", "time since reset", cmd_uptime},
    {"pi_start", "start pi calculation: pi_start [terms]", cmd_pi_start},
    {"pi", "pi calculation state or result", cmd_pi},
    {"profiling", "show stopwatches", cmd_profiling},
    {"profiling_reset", "reset stopwatches", cmd_profiling_reset},
    {"led_enable", "turn LED on", cmd_led_enable},
    {"led_disable", "turn LED off", cmd_led_disable},
    {"led_blink", "blink LED", cmd_led_blink},
    {"led_period", "set blink period: led_period <period_ms>", cmd_led_period},
    {"button", "button state and press count", cmd_button},
    {"edge", "rise and fall of the RC line: edge pp|od", cmd_edge},
    {"loop", "UART0 loopback: loop internal|pins", cmd_loop},
    {"echo", "text through UART0 and back: echo <text>", cmd_echo},
};

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);