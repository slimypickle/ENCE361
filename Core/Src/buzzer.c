/*
 * buzzer.c
 *
 * Non-blocking piezoelectric buzzer driver.
 *
 * PD0 is toggled every 1 ms to produce a ~500 Hz square wave.
 * The alert lasts BUZZ_DURATION_MS milliseconds.
 *
 * GPIOD clock and PD0 output init are performed in gpio.c.
 */

#include "buzzer.h"
#include "main.h"                   /* GPIO port/pin macros               */
#include "stm32c0xx_hal.h"
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/* Configuration                                                        */
/* ------------------------------------------------------------------ */
#define BUZZ_TOGGLE_PERIOD_MS   1U      /* toggle every 1 ms → 500 Hz     */
#define BUZZ_DURATION_MS        500U    /* alert lasts 500 ms              */

/* ------------------------------------------------------------------ */
/* Private state                                                        */
/* ------------------------------------------------------------------ */
static bool     s_active       = false;
static uint32_t s_end_tick     = 0;
static uint32_t s_last_toggle  = 0;

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */
void buzzer_init(void)
{
    s_active = false;
    HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
}

void buzzer_trigger(void)
{
    s_active      = true;
    s_end_tick    = HAL_GetTick() + BUZZ_DURATION_MS;
    s_last_toggle = HAL_GetTick();
    HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
}

void buzzer_task_execute(void)
{
    if (!s_active) return;

    uint32_t now = HAL_GetTick();

    /* Stop when duration has elapsed */
    if (now >= s_end_tick)
    {
        s_active = false;
        HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
        return;
    }

    /* Toggle at the configured period to generate the tone */
    if (now - s_last_toggle >= BUZZ_TOGGLE_PERIOD_MS)
    {
        s_last_toggle = now;
        HAL_GPIO_TogglePin(Buzzer_GPIO_Port, Buzzer_Pin);
    }
}
