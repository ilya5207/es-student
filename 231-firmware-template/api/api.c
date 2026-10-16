#include "api/api.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include "device/device.h"
#include "profiling/profiling.h"
#include "pico/stdlib.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "firmware.h"

// обработчик команды: получает имя и аргументы
typedef void (*api_callback_t)(const command_t *command);

// команда прибора: имя, справка для help и обработчик
typedef struct
{
    const char *name;
    const char *help;
    api_callback_t callback;
} api_command_t;

static void cmd_info(const command_t *command)
{
        (void)command;
        device_info_t info;
        device_get_info(&info);
        printf("device name: %s\n", DEVICE_NAME);
        printf("firmware version: %s\n", FIRMWARE_VERSION);    
        printf("project: %s\n", DEVICE_PROJECT);
        printf("repo: %s\n", DEVICE_REPO);
        printf("board: %s\n", info.board);
        printf("serial: %s\n", info.serial);
        printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n", (unsigned)info.manufacturer, (unsigned)info.part, (unsigned)info.revision);
        printf("pico-sdk: %s\n", info.sdk_version);    
}

static void cmd_uptime(const command_t *command)
{
    (void)command;
    printf("uptime: %llu ms\n", time_us_64() / 1000);
}

// Число без знака из десятичной записи. false — в записи не только цифры
// или число не помещается в uint32_t.
static bool parse_u32(const char *text, uint32_t *value)
{
    // strtoul приняла бы и знак: из "-1" получилось бы 4294967295
    if (!isdigit((unsigned char)text[0]))
    {
        return false;
    }

    char *end;
    errno = 0;
    unsigned long number = strtoul(text, &end, 10);

    // после числа остались символы или число не поместилось
    if (*end != '\0' || errno == ERANGE || number > UINT32_MAX)
    {
        return false;
    }

    *value = (uint32_t)number;
    return true;
}

static void cmd_pi_start(const command_t *command)
{
    uint32_t terms = 1000000;
    if (command->argc > 1) 
    {
        printf("error: usage pi_start [terms]\n");
        return;
    }

    if (command->argc ==1)
    {
        if(!parse_u32(command->argv[0], &terms) || terms == 0)
        {
           if(terms == 0) printf("error: terms must be greater than 0\n");
           else printf("error: usage pi_start [terms]\n");    
           return;
        }
    }

    if (pi_task_start(terms))
    {
        printf("pi: started, %lu terms\n", (unsigned long)terms);
    }
    else
    {
        printf("error: pi is busy\n");
    }
    
}

static void cmd_pi(const command_t *command)
{
    double result = 0.0;
    uint32_t time_ms = 0;
    pi_task_state_t pi_state = pi_task_get_result(&result, &time_ms);

    if (pi_state == PI_TASK_NOT_STARTED)
    {
        printf("pi: not started\n");
    }    
    else if (pi_state == PI_TASK_RUNNING)
    {
           printf("pi: running\n");
    } 
    else if (pi_state == PI_TASK_READY)
    {
        printf("pi: %.9f in %lu ms\n", result, (unsigned long)time_ms);    
    }
}

static void cmd_profiling(const command_t *command)
{
        (void)command;
        for (uint32_t id = 0; id < PROFILING_STOPWATCH_COUNT; id++)
        {
            profiling_result_t result;
            if (profiling_get(id, &result))
            {
                printf("%-10s count %-10u mean %.2f us max %u us\n", result.name, (unsigned)result.count, result.mean_us, (unsigned)result.max_us);
            }
        }       
}

static void cmd_profiling_reset(const command_t *command)
{
    (void)command;
    profiling_reset();
    printf("profiling reset\n");
}

static void print_led_state(void)
{
    const char *state_str = "unknown";
    switch (led_task_get_state())
    {
    case LED_STATE_OFF:
        state_str = "0ff";
        break;
    case LED_STATE_ON:
        state_str = "on";
        break;
    case LED_STATE_BLINK:
        state_str = "blink";
        break;        
    }
    
    printf("led: %s, period %lu ms\n", state_str, led_task_get_period_ms());
}

static void cmd_led_enable(const command_t *command)
{
    led_task_set_state(LED_STATE_ON);
    print_led_state();
}

static void cmd_led_disable(const command_t *command)
{
    led_task_set_state(LED_STATE_OFF);
    print_led_state();
}

static void cmd_led_blink(const command_t *command)
{
    led_task_set_state(LED_STATE_BLINK);
    print_led_state();
}

static void cmd_led_period(const command_t *command)
{
    uint32_t period;

    if (command->argc != 1)
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }
    
    if (!parse_u32(command->argv[0], &period))
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }
    if (!led_task_set_period_ms(period))
    {
        printf("error: period_ms must be greater than 0\n");
        return;
    }
    print_led_state();
}

static void cmd_button(const command_t *command)
{
    (void)command;
    printf("button: %s, presses %u\n", button_task_is_pressed() ? "pressed" : "released", (unsigned)button_task_get_press_count());
}

const api_command_t commands[] = {
    {"info", "device passport", cmd_info},
    {"uptime", "time since reset", cmd_uptime},
    {"pi_start", "start pi calculation: pi_start [terms]", cmd_pi_start},
    {"pi", "pi calculation state or result", cmd_pi},
    {"profiling", "show stopwatches", cmd_profiling},
    {"profiling_reset", "reset stopwatches", cmd_profiling_reset},
    {"led_enable", "Turn LED on", cmd_led_enable},
    {"led_disable", "Turn LED off", cmd_led_disable},
    {"led_blink", "Blink LED", cmd_led_blink},
    {"led_period", "Set LED blink period in ms", cmd_led_period},
    {"button", "button state and press count", cmd_button},
};

static const uint api_command_count = sizeof(commands) / sizeof(commands[0]);

void api_handle(const command_t *command)
{
    if (command->truncated)
    {
        printf("error: command longer than 63 characters or 4 arguments\n");
        return;
    }

    if (strcmp(command->name, "help") == 0)
    {
        printf("%-20s list of commands\n", "help");
        for (uint i = 0; i < api_command_count; ++i)
        {
            printf("%-20s %s\n", commands[i].name, commands[i].help);
        }
        return;
    }

    for (uint32_t i = 0; i < api_command_count; ++i)
    {
         if (strcmp(commands[i].name, command->name) == 0)
         {
             commands[i].callback(command);
             return;
         }
    }

printf("error: unknown command '%s', try help\n", command->name);
}