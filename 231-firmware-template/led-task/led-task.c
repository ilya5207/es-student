#include "led-task/led-task.h"
#include "pico/stdlib.h"

static led_state_t current_state;
static uint32_t blink_period_ms;
static uint64_t last_toggle_us;

static bool period_elapsed(uint64_t *last_us, uint64_t period_us)
{
    uint64_t now_us = time_us_64();
    if (now_us - *last_us >= period_us)
    {
        if (now_us - *last_us > 2 * period_us)
        {
            *last_us = now_us;
        }
        else
        {
            *last_us += period_us;
        }
        return true;
    }
    return false;
}

void led_task_init(void)
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    blink_period_ms = 1000;
    led_task_set_state(LED_STATE_BLINK);
}

void led_task_set_state(led_state_t state)
{
    current_state = state;

    switch (current_state)
    {
    case LED_STATE_OFF:
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        break;
    case LED_STATE_ON:
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        break;
    case LED_STATE_BLINK:
        last_toggle_us = time_us_64();
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        break;    
    }
}

void led_task_handle(void)
{
    switch (current_state)
    {
    case LED_STATE_OFF:
    case LED_STATE_ON:
        
        break;
    
    case LED_STATE_BLINK:
    if (period_elapsed(&last_toggle_us, (blink_period_ms * 1000ULL) / 2))
    {
        gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
    }
        break;
    }
}

bool led_task_set_period_ms(uint32_t period_ms)
{
    if (period_ms == 0) return false;
    blink_period_ms = period_ms;
    return true;
}

uint32_t led_task_get_period_ms(void)
{
    return blink_period_ms;
}

led_state_t led_task_get_state(void)
{
    return current_state;
}

void led_task_next_state(void)
{
    switch (current_state)
    {
    case LED_STATE_OFF:
        led_task_set_state(LED_STATE_ON);
        break;
    case LED_STATE_ON:
        led_task_set_state(LED_STATE_BLINK);    
        break;
    case LED_STATE_BLINK:
        led_task_set_state(LED_STATE_OFF);
        break;
    }
}