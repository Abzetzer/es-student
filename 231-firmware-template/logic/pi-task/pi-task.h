#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    PI_STATE_NOT_STARTED,
    PI_STATE_PROCESS,
    PI_STATE_FINISH,
} pi_state_t;

void pi_task_init(void);

bool pi_task_start(uint32_t terms);

void pi_task_handle(void);

typedef struct
{
    double pi;
    uint64_t time;
    pi_state_t state;
} pi_task_result_t;

const pi_task_result_t pi_task_get_result(void);