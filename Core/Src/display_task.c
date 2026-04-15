/*
 * display_task.c
 *
 * Serial UART debug output (M1.2 / M1.3).
 *
 * When UART output is enabled it prints raw joystick ADC values and
 * the current step count at 4 Hz.  The OLED display is driven
 * entirely by ui_task.c.
 */

#include "display_task.h"
#include "usart.h"
#include "joystick_task.h"
#include "step_counter.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "stm32c0xx_hal.h"

/* ------------------------------------------------------------------ */
/* Configuration                                                        */
/* ------------------------------------------------------------------ */
#define DISPLAY_TASK_PERIOD_MS  250U

/* ------------------------------------------------------------------ */
/* Private state                                                        */
/* ------------------------------------------------------------------ */
static uint32_t s_next_run    = 0;
static bool     s_uart_enabled = true;

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */
void display_task_toggle_uart(void)
{
    s_uart_enabled = !s_uart_enabled;
}

void display_task_init(void)
{
    s_next_run = HAL_GetTick() + DISPLAY_TASK_PERIOD_MS;
}

void display_task_execute(void)
{
    uint32_t now = HAL_GetTick();
    if (now < s_next_run) return;
    s_next_run += DISPLAY_TASK_PERIOD_MS;

    if (!s_uart_enabled) return;

    char buf[48];
    snprintf(buf, sizeof(buf), "X:%4d Y:%4d Steps:%lu\r\n",
             joystick_get_x_raw(),
             joystick_get_y_raw(),
             (unsigned long)sc_get_steps());
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
}
