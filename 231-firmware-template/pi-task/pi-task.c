#include "pi-task/pi-task.h"
#include "pico/time.h"

#define PI_TERMS_PER_CALL 100

static pi_task_state_t pi_state;
static uint32_t pi_n;
static uint32_t pi_k;
static double pi_sum;
static double pi_sign;
static uint64_t start_time_us;
static uint64_t end_time_us;

bool pi_task_start(uint32_t terms)
{
    if (pi_state == PI_TASK_RUNNING)
    {
        return false;
    }

    pi_n = terms;
    pi_k = 0;
    pi_sum = 0.0;
    pi_sign = 1.0;
    start_time_us = time_us_64();
    pi_state = PI_TASK_RUNNING;
    return true;
}

void pi_task_init(void)
{
    pi_state = PI_TASK_NOT_STARTED;
}

void pi_task_handle(void)
{
    if (pi_state != PI_TASK_RUNNING)
    {
        return;
    }
    
    uint32_t end = pi_k + PI_TERMS_PER_CALL;
    if (end < pi_k || end > pi_n)
    {
        end = pi_n;
    }

    for (; pi_k < end; pi_k++)
    {
        pi_sum += pi_sign / (2.0 * pi_k + 1.0);
        pi_sign = -pi_sign;
    }

    if (pi_k == pi_n)
    {
        end_time_us = time_us_64();
        pi_state = PI_TASK_READY;
    }
}

pi_task_state_t pi_task_get_result(double *result, uint32_t *time_ms)
{
    if (pi_state == PI_TASK_READY)
    {
        *result = 4.0 * pi_sum;
        *time_ms = (end_time_us - start_time_us) / 1000;
    }
    return pi_state;
}