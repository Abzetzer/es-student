#pragma once

#include <stdbool.h>
#include <stdint.h>

void pi_task_init(void);

bool pi_task_start(uint32_t terms);

void pi_task_handle(void);

bool pi_task_get_result(double *pi, uint32_t *t);

uint32_t pi_task_calc_terms(void);