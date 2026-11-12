#include "api/api-commands.h"

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "device/device.h"
#include "profiling/profiling.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "firmware.h"
#include "systime/systime.h"
#include "platform/platform.h"
#include "block-task/block-task.h"
#include <math.h>

#define CALC_PI_DEFAULT_TERMS 1000000u
#define TELEMETRY_DEFAULT_PERIOD_MS 100
#define STREAM_BLINK_PERIOD_MS 250
#define MIC_DEFAULT_WINDOWS_MS 100

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

static void cmd_get_voltage(const command_t *command)
{
    (void)command;
    platform_voltage_read();
    printf("voltage: %.3f V\n", platform_voltage_read());
}

static void cmd_get_temp(const command_t *command)
{
    (void)command;
    platform_temp_read();
    printf("temp: %.1f C\n", platform_temp_read());
}

void api_print_telemetry(const telemetry_record_t *record)
{
    printf("tm: %llu %.3f %.1f\n", (unsigned long long)record->time_us, record->voltage_v, record->temp_c);
}

static void command_tm_start(const command_t *command)
{
    uint32_t period_ms = TELEMETRY_DEFAULT_PERIOD_MS;
    if (command->argc > 1 || (command->argc == 1 && !parse_u32(command->argv[0], &period_ms)))
    {
        printf("error: usage tm_start [<period_ms>]\n");
        return;
    }
    if (!telemetry_task_start(period_ms))
    {
        printf("error: period_ms must be at least 10\n");
        return;
    }
    printf("telemetry: started, period %u ms\n", (unsigned)period_ms);
}

static void command_tm_stop(const command_t *command)
{
    (void)command;
    telemetry_task_stop();
    printf("telemetry: stopped\n");
}

static void command_charge_on(const command_t *command)
{
    (void)command;
    platform_charge_set(true);
    printf("charge: on\n");
}
static void command_charge_off(const command_t *command)
{
    (void)command;
    platform_charge_set(false);
    printf("charge: off\n");
}

static void cmd_timer_blink_start(const command_t *command)
{
    uint32_t period_ms;
    if (command->argc != 1 || !parse_u32(command->argv[0], &period_ms))
    {
        puts("error: usage timer_blink_start <period_ms>");
        return;
    }
    if (period_ms < 10 || period_ms > 60000)
    {
        puts("error: period_ms must be 10..60000");
        return;
    }
    led_task_set_state(LED_STATE_OFF);
    if (!platform_blink_start(period_ms))
    {
        puts("error: timer unavailable");
        return;
    }
    printf("timer_blink: started, period %u ms\n", (unsigned)period_ms);
}

static void cmd_timer_blink_stop(const command_t *command)
{
    (void)command;
    printf("timer_blink: stopped, %u toggles\n", (unsigned)platform_blink_stop());
}

static void cmd_cap_start(const command_t *command)
{
    uint32_t count, period_us;
    if (command->argc != 2 || !parse_u32(command->argv[0], &count) || !parse_u32(command->argv[1], &period_us))
    {
        puts("error: usage cap_start <N> <period_us>\n");
        return;
    }
    if (count == 0 || count > PLATFORM_CAPTURE_MAX || period_us < 10)
    {
        puts("error: N must be 1..2000, period_us at least 10\n");
        return;
    }
    if (telemetry_task_get_state() == TELEMETRY_STATE_RUN || !platform_capture_start(count, period_us))
    {
        puts("error: adc_busy\n");
        return;
    }
    printf("capture: started, %u samples, period %u us\n", (unsigned)count, (unsigned)period_us);
}

static void cmd_cap_dump(const command_t *command)
{
    (void)command;
    if (platform_adc_busy())
    {
        puts("error: adc busy\n");
        return;
    }
    if (!platform_capture_done())
    {
        puts("error: no capture\n");
        return;
    }
    platform_capture_t capture;
    platform_capture_get(&capture);
    printf("capture: %u samples, period %u us, interval %u..%u us, handler %.1f %%\n", (unsigned)capture.count, (unsigned)capture.period_us, (unsigned)capture.min_interval_us, (unsigned)capture.max_interval_us, (double)capture.handler_percent);
    for (uint32_t i = 0; i < capture.count; i++)
    {
        if (i % 16 == 0)
        {
            printf("cap:");
        }    
        printf(" %u", (unsigned)capture.samples[i]);
        if (i % 16 == 15 || i + 1 == capture.count)
        {
            printf("\n");
        }        
    }
}

static void print_dma_cap(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    uint16_t minimum = samples[0];
    uint16_t maximum = samples[0];
    uint32_t sum = 0;

    for (uint32_t i = 0; i < count; i++)
    {
        if (samples[i] < minimum)
        {
            minimum = samples[i];
        }
        if (samples[i] >maximum)
        {
            maximum = samples[i];
        }
        sum += samples[i];
    }

    uint64_t expected_us = (uint64_t)count * 1000000u / PLATFORM_SAMPLE_RATE_HZ;
    printf("dma_cap: %u samples in %u us, expected %llu us, min %u max %u mean %.1f\n", (unsigned)count, (unsigned)time_us, (unsigned long long)expected_us, (unsigned)minimum, (unsigned)maximum, (double)sum / count);
}

static void cmd_dma_cap(const command_t *command)
{
    uint32_t count;

    if (command->argc != 1 || !parse_u32(command->argv[0], &count))
    {
        printf("error: usage dma_cap <N>\n");
        return;
    }

    if (count == 0 || count > PLATFORM_BLOCK_MAX)
    {
        printf("error: N must be 1..8192\n");
        return;
    }

    if (telemetry_task_get_state() == TELEMETRY_STATE_RUN || !block_task_start(count, print_dma_cap))
    {
        printf("error: adc busy\n");
    }
}

void api_send_frame(const stream_header_t *header, const uint8_t *data, uint32_t bytes)
{
    platform_write(header, sizeof(*header));
    platform_write(data, bytes);
}

static void cmd_stream_start(const command_t *command)
{
    uint32_t bits = 16;
    if (command->argc > 1 || (command->argc == 1 && !parse_u32(command->argv[0], &bits)) || (bits != 8 && bits != 16))
    {
        printf("error: usage stream_start [8|16]\n");
        return;
    }

    if (telemetry_task_get_state() == TELEMETRY_STATE_RUN || !stream_task_start((uint8_t)bits))
    {
        printf("error: adc busy\n");
        return;
    }

    printf("stream: started, %u Hz, %u bit\n", (unsigned)PLATFORM_SAMPLE_RATE_HZ, (unsigned)bits);
    platform_blink_start(STREAM_BLINK_PERIOD_MS);
    led_task_set_state(LED_STATE_OFF);
}

static void cmd_stream_stop(const command_t *command)
{
    (void)command;
    
    if (stream_task_get_state() == STREAM_STATE_RUN)
    {
        stream_task_stop();
        platform_blink_stop();
    }

    printf("stream: stopped, %u frames, %u samples, %u overruns\n", (unsigned)stream_task_get_frames(), (unsigned)stream_task_get_samples(), (unsigned)stream_task_get_overruns());
}

static void cmd_generate_tone(const command_t *command)
{
    uint32_t freq_hz;

    if (command->argc != 1 || !parse_u32(command->argv[0], &freq_hz))
    {
        printf("error: usage generate_tone <Hz>\n");
        return;
    }

    if (!platform_tone_set(freq_hz))
    {
        printf("error: Hz must be 0 or 20..20000\n");
        return;
    }

    if (freq_hz == 0)
    {
        printf("tone: off\n");
    }
    else
    {
        printf("tone: %u Hz\n", (unsigned)freq_hz);
    }
}

static void cmd_hang(const command_t *command)
{
    printf("hang: superloop stopped\n");

    while (1)
    {
    }
}

static void cmd_wdt_enable(const command_t *command)
{
    uint32_t timeout_ms;

    if (command->argc != 1 || !parse_u32(command->argv[0], &timeout_ms))
    {
        printf("error: usage wdt_enable <timeout_ms\n>");
        return;
    }

    if (!platform_watchdog_enable(timeout_ms))
    {
        printf("error: timeout_ms must be 1..8388\n");
        return;
    }

    printf("wdt: enabled, timeout %u ms\n", (unsigned)timeout_ms);
}

static void cmd_reset_reason(const command_t *command)
{
    (void)command;
    uint32_t restarts;
    bool watchdog_reset = platform_watchdog_caused_reset(&restarts);
    printf("reset: %s, restarts %u\n", watchdog_reset ? "watchdog" : "other", (unsigned)restarts);
}

static void block_stats(const uint16_t *samples, uint32_t count, float *mean, float *rms, float *peak)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        sum += samples[i];
    }
    uint32_t center = (int32_t)(sum / count);

    uint64_t squares = 0;
    uint32_t largest = 0;
    for ( uint32_t i = 0; i < count; i++)
    {
        int32_t deviation = (int32_t)samples[i] - center;
        uint32_t size = deviation < 0 ? -deviation : deviation;
        squares += (uint64_t)(deviation * deviation);
        if (size > largest)
        {
            largest = size;
        }
    }

    *mean = (float)sum / count;
    *rms = sqrtf((float)squares / count);
    *peak = (float)largest; 
}

static void print_mic_bias(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    (void)time_us;
    float mean, rms, peak;
    block_stats(samples, count, &mean, &rms, &peak);
    float volts = platform_adc_volts(mean);
    printf("mic: bias %.3f V\n", volts);
}

static void print_mic_level(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    (void)time_us;
    float mean, rms, peak;
    block_stats(samples, count, &mean, &rms, &peak);
    float rms_mv = rms * 1000 * ADC_VREF_V / ADC_LEVELS;
    float peak_mv = peak * 1000 * ADC_VREF_V / ADC_LEVELS;
    uint32_t window_ms = (uint32_t)((uint64_t)count * 1000 / PLATFORM_SAMPLE_RATE_HZ);
    printf("mic: level %.1f mV rms, peak %.1f mV, %u ms\n", (double)rms_mv, (double)peak_mv, (unsigned)window_ms);
}

static void start_block(uint32_t count, block_task_callback_t callback)
{
    if (telemetry_task_get_state() == TELEMETRY_STATE_RUN)
    {
        printf("error: adc busy\n");
        return;
    }
    if (!block_task_start(count, callback))
    {
        printf("error: adc busy\n");
        return;
    }
}

static void cmd_mic_bias(const command_t *command)
{
    (void)command;
    start_block(4410, print_mic_bias);
}

static void cmd_mic_level(const command_t *command)
{
    uint32_t ms = MIC_DEFAULT_WINDOWS_MS;
    if (command->argc > 1)
    {
        printf("error: usage mic_level [<ms>]\n");
        return;
    }
    if (command->argc == 1)
    {
        char *endptr;
        long val = strtol(command->argv[0], &endptr, 10);
        if (*endptr != '\0')
        {
            printf("error: usage mic_level [<ms>]\n");
            return;
        }
        ms = (uint32_t)val;
    }
    if (ms == 0 || ms > 185)
    {
        printf("error: ms must be 1..185\n");
        return;
    }
    uint32_t count = (uint32_t)((uint64_t)ms * PLATFORM_SAMPLE_RATE_HZ / 1000);
    start_block(count, print_mic_level);
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
    {"get_voltage", "voltage on GP26, ADC input 0", cmd_get_voltage},
    {"get_temp", "chip temperature, degrees Celsius", cmd_get_temp},
    {"tm_start", "start telemetry: tm_start [<period_ms>]", command_tm_start},
    {"tm_stop", "stop telemetry", command_tm_stop},
    {"charge_on", "charge RC circuit on GP22", command_charge_on},
    {"charge_off", "discharge RC circuit", command_charge_off},
    {"timer_blink_start", "blink led by timer: timer_blink_start <period_ms>", cmd_timer_blink_start},
    {"timer_blink_stop", "stop timer blink, number of toggles", cmd_timer_blink_stop},
    {"cap_start", "capture ADC: cap_start <N> <period_us>", cmd_cap_start},
    {"cap_dump", "capture samples and timer intervals", cmd_cap_dump},
    {"dma_cap", "capture ADC through DMA: dma_cap <N>", cmd_dma_cap},
    {"stream_start", "start ADC stream: stream_start [8|16]", cmd_stream_start},
    {"stream_stop", "stop ADC stream, counters and overruns", cmd_stream_stop},
    {"generate_tone", "generate test tone: generate_tone <Hz>", cmd_generate_tone},
    {"hang", "stop superloop until reset", cmd_hang},
    {"wdt_enable", "enable watchdog: wdt_enable <timeout_ms>", cmd_wdt_enable},
    {"reset_reason", "last reset cause and restart count", cmd_reset_reason},
    {"mic_bias", "microphone bias voltage", cmd_mic_bias},
    {"mic_level", "microphone level: mic_level [<ms>]", cmd_mic_level},
};

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);