/*
 * button_task.c
 *
 * Handles button inputs for Milestone 2:
 *
 *   SW1 (UP)    : Increase DS3 LED brightness by 10 %  (M1.5 retained)
 *   SW2 (DOWN)  : Toggle UART serial output  (M1.3 retained)
 *                 Rapid double-tap (< 500 ms between presses) toggles
 *                 test mode  (M2.3)
 *   SW3 (RIGHT) : Toggle RGB RIGHT LED  (M1 retained)
 *   SW4 (LEFT)  : Add 7 to step count  (M2.2)
 */

#include "button_task.h"
#include "buttons.h"
#include "step_counter.h"
#include "display_task.h"
#include "rgb.h"
#include "pwm.h"
#include "tim.h"
#include "stm32c0xx_hal.h"

/* ------------------------------------------------------------------ */
/* Configuration                                                        */
/* ------------------------------------------------------------------ */
#define BUTTON_TASK_PERIOD_MS   20U
#define PWM_STEP_PERCENT        10U

/* Double-tap detection window (ms).  Two presses of SW2 within this
   window toggle test mode.                                             */
#define DOUBLE_TAP_WINDOW_MS    500U

/* ------------------------------------------------------------------ */
/* Private state                                                        */
/* ------------------------------------------------------------------ */
static uint32_t s_next_run       = 0;
static uint8_t  s_duty           = 0;    /* DS3 starts off (0 %) at reset */
static bool     s_test_mode      = false;
static uint8_t  s_sw2_tap_count  = 0;     /* consecutive taps            */
static uint32_t s_sw2_first_tick = 0;     /* tick of first tap           */

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */
void button_task_init(void)
{
    buttons_init();
    s_duty           = 0;              /* explicitly reset to 0 % so that
                                          soft-resets (which may not re-run
                                          C startup) start DS3 off         */
    pwm_setDutyCycle(&htim2, TIM_CHANNEL_3, s_duty);
    s_next_run       = HAL_GetTick() + BUTTON_TASK_PERIOD_MS;
    s_sw2_tap_count  = 0;
    s_test_mode      = false;
}

void button_task_execute(void)
{
    uint32_t now = HAL_GetTick();
    if (now < s_next_run) return;
    s_next_run += BUTTON_TASK_PERIOD_MS;

    buttons_update();

    /* ---- SW1 (UP): increase DS3 brightness by 10 %, wraps 100→0 % - */
    if (buttons_checkButton(UP) == PUSHED)
    {
        s_duty = (s_duty >= 100) ? 0 : s_duty + PWM_STEP_PERCENT;
        pwm_setDutyCycle(&htim2, TIM_CHANNEL_3, s_duty);
    }

    /* ---- SW2 (DOWN): UART toggle + double-tap test mode ------------ */
    if (buttons_checkButton(DOWN) == PUSHED)
    {
        /* Single-press always toggles UART (M1.3 retained) */
        display_task_toggle_uart();

        /* Double-tap detection */
        if (s_sw2_tap_count == 0)
        {
            /* First tap */
            s_sw2_tap_count  = 1;
            s_sw2_first_tick = now;
        }
        else
        {
            /* Second tap – check it's within the double-tap window */
            if (now - s_sw2_first_tick <= DOUBLE_TAP_WINDOW_MS)
            {
                s_test_mode     = !s_test_mode;
                s_sw2_tap_count = 0;
            }
            else
            {
                /* Too slow: treat this press as a new first tap */
                s_sw2_tap_count  = 1;
                s_sw2_first_tick = now;
            }
        }
    }

    /* Reset tap counter if the window has expired */
    if (s_sw2_tap_count == 1 &&
        (now - s_sw2_first_tick) > DOUBLE_TAP_WINDOW_MS)
    {
        s_sw2_tap_count = 0;
    }

    /* ---- SW3 (RIGHT): toggle RGB RIGHT LED ------------------------- */
    if (buttons_checkButton(RIGHT) == PUSHED)
    {
        rgb_led_toggle(RGB_RIGHT);
        rgb_colour_all_on();
    }

    /* ---- SW4 (LEFT): increment step count by 7  (M2.2) ------------ */
    if (buttons_checkButton(LEFT) == PUSHED)
    {
        sc_add_steps(7);
    }
}

bool button_task_is_test_mode(void)
{
    return s_test_mode;
}
