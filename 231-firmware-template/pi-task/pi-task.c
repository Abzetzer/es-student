#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include "button-task.h"
#include "led-task.h"
#include "pi-task.h"


static const uint32_t CALC_PI_TERMS = 1000000;
static const uint32_t PI_TERMS_PER_CALL = 100;

typedef enum{
    PI_STATE_NO, 
    PI_STATE_COUNT, 
    PI_STATE_RESULT, 
} pi_state_t;

static pi_state_t pi_state;

static volatile double pi_result;

static uint64_t start_us;

static uint64_t calc_us;


static uint32_t pi_terms;

static uint32_t pi_terms_end;

static double pi_sum;
static double pi_sign;

static bool isCountResult = false;

void pi_task_init(void) {
    pi_state    = PI_STATE_NO;
    pi_result   = 0; // вычисляется в pi_task_handle
    start_us    = 0; // устанавливается в pi_task_start
    calc_us     = 0; // меняется в pi_task_handle
    pi_terms    = 0; // меняется в pi_task_handle
    pi_terms_end= 0; // устанавливается в pi_task_start
    pi_sum      = 0; // вычисляется в pi_task_handle
    pi_sign     = 1; // вычисляется в pi_task_handle
}

// Состояние расчёта: должно пережить возврат из обработчика
/*bool pi_running = false;
bool pi_ready = false;
int pi_k;
int pi_n;
double pi_sum;
double pi_sign;
double pi_result;*/

bool pi_task_start(uint32_t terms)
{
    // Вторые вычисления не начинаются, пока не закончились предыдущие
    if (pi_state == PI_STATE_COUNT) 
    return false;

    // Начальные значения для вычисления
    pi_task_init();
    // Количество шагов вычисления
    pi_terms_end = terms;

    // Запомнить время вычисления
    start_us = time_us_64(); // Замер монотонного времени
    calc_us  = start_us;

    // Запустить вычисление
    pi_state = PI_STATE_COUNT;
    return true;
}

void pi_task_handle(void)
{
    switch (pi_state) {
        // Ничего не делаем
        case PI_STATE_NO:
            break;
        case PI_STATE_RESULT: 
            break;
        // Порционное вычисление, запоминание времени
        case PI_STATE_COUNT: {
            uint32_t end = pi_terms + PI_TERMS_PER_CALL;
            if (end > pi_terms_end) end = pi_terms_end; // Вычисления подошли к концу
            if (end < pi_terms)     end = pi_terms_end; // Переполнение, больше 4 294 967 295
            
            // Цикл с порционным вычислением
            while (pi_terms < end) {
                // Вычисление
                pi_sum +=  pi_sign / (2.0 * pi_terms + 1.0);
                pi_sign = -pi_sign;
                // Увеличения счетчика итераций
                pi_terms++;
                            // Возврат промежуточных значений
            if (isCountResult) {
                // Промежуточное (итоговое) сохранение результата
                pi_result = 4.0 * pi_sum;
                // Сохраняем время
                calc_us = time_us_64(); // Замер монотонного времени
            }

            // Вычисления закончились    
            if (pi_terms == pi_terms_end) {
                // Промежуточное (итоговое) сохранение результата
                pi_result = 4.0 * pi_sum;
                // Сохраняем время
                calc_us = time_us_64(); // Замер монотонного времени
                // Конец вычислений
                pi_state = PI_STATE_RESULT;
            }
            break;
            }
        }
    }
}

// Готов ли результат: если да, записать его по указателю


// Возвращает значение и время, потраченное на расчет (false - расчет на начат)
// pi = 0, t = 0 - идут вычисления (либо промежуточные значения, либо "занят")
bool pi_task_get_result(double *pi, uint32_t *t) {
    // переходы по таблице; 
    switch (pi_state) {
        case PI_STATE_NO: return false;
        case PI_STATE_COUNT: { // Занят
            if (!isCountResult) if (pi) *pi = 0;
            if (!isCountResult) if (t ) *t  = 0;
            if (!isCountResult) return true;
        }
        case PI_STATE_RESULT: {
            // Значение пи
            if (pi) *pi = pi_result;
            // Сремя с момента начала вычисления
            if (t ) *t  = (uint32_t)((calc_us - start_us)/1000);
            return true;
        }
    }//switch
    return false;
}

// Возвращает точность вычислений по умолчанию
uint32_t pi_task_calc_terms(void) {
    return CALC_PI_TERMS;
}