/*
 * buzzer.c
 *
 * Non-blocking piezo buzzer driver. Toggles PD0 at a fixed rate to
 * produce an audible tone, for a fixed total duration.
 */

#include "buzzer.h"
#include "main.h"
#include "stm32c0xx_hal.h"
#include <stdbool.h>

/* Toggle every 2 ms = 4 ms period = 250 Hz square wave. 250 Hz is
 * loud enough on the RCAP piezo and well within human hearing. */
#define BUZZ_TOGGLE_PERIOD_MS   2U
#define BUZZ_DURATION_MS        1000U

static bool     s_active      = false;
static uint32_t s_end_tick    = 0;
static uint32_t s_last_toggle = 0;

void buzzer_init(void)
{
    s_active = false;
    HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
}

void buzzer_trigger(void)
{
    /* Calling this while the buzzer is already sounding just restarts
     * the duration timer — which is what the user probably expects. */
    uint32_t now  = HAL_GetTick();
    s_active      = true;
    s_end_tick    = now + BUZZ_DURATION_MS;
    s_last_toggle = now;
    HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
}

void buzzer_task_execute(void)
{
    if (!s_active) return;

    uint32_t now = HAL_GetTick();

    /* End of the alert — leave the pin low so the piezo is silent. */
    if (now >= s_end_tick)
    {
        s_active = false;
        HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
        return;
    }

    if (now - s_last_toggle >= BUZZ_TOGGLE_PERIOD_MS)
    {
        s_last_toggle = now;
        HAL_GPIO_TogglePin(Buzzer_GPIO_Port, Buzzer_Pin);
    }
}
