#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PI_TASK_NOT_STARTED,
    PI_TASK_RUNNING,
    PI_TASK_READY,
} pi_task_state_t;

bool pi_task_start(uint32_t terms);
void pi_task_handle(void);
void pi_task_init(void);
pi_task_state_t pi_task_get_result(double *result, uint32_t *time_ms);