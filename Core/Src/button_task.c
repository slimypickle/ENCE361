/*
 * button_task.c
 *
 * Handles button inputs
 */

#include "button_task.h"
#include "buttons.h"
#include "step_counter.h"
#include "stm32c0xx_hal.h"


/* Configuration                                                        */

#define BUTTON_TASK_PERIOD_MS   20U

/* Double-tap detection window (ms). Two presses of SW2 within this
   window toggle test mode (Spec §7).                                   */
#define DOUBLE_TAP_WINDOW_MS    500U

/* SW4 manual step nudge — kept small so it's a safety net, not a
   demo-disrupting feature.                                             */
#define SW4_STEP_NUDGE          0


/* Private state                                                        */

static uint32_t s_next_run       = 0;
static bool     s_test_mode      = false;
static uint8_t  s_sw2_tap_count  = 0;
static uint32_t s_sw2_first_tick = 0;


/* Public API                                                           */

void button_task_init(void)
{
    buttons_init();
    /* DS3 PWM is owned by led_progress_init().  Do NOT touch
     * TIM_CHANNEL_3 here.                                            */
    s_next_run       = HAL_GetTick() + BUTTON_TASK_PERIOD_MS;
    s_sw2_tap_count  = 0;
    s_sw2_first_tick = 0;
    s_test_mode      = false;
}

void button_task_execute(void)
{
    uint32_t now = HAL_GetTick();
    if (now < s_next_run) return;
    s_next_run += BUTTON_TASK_PERIOD_MS;

    buttons_update();

    /* SW1 (UP): unassigned, consume event so debounce FSM stays
     *                 clean even though we ignore the result.          */
    (void)buttons_checkButton(UP);

    /* SW2 (DOWN): double-tap toggles test mode (Spec 7) -------- */
    if (buttons_checkButton(DOWN) == PUSHED)
    {
        if (s_sw2_tap_count == 0)
        {
            /* First tap of a potential double-tap sequence */
            s_sw2_tap_count  = 1;
            s_sw2_first_tick = now;
        }
        else
        {
            /* Second tap — does it fall inside the window? */
            if ((now - s_sw2_first_tick) <= DOUBLE_TAP_WINDOW_MS)
            {
                s_test_mode     = !s_test_mode;
                s_sw2_tap_count = 0;
            }
            else
            {
                /* Too slow — restart the sequence with this tap */
                s_sw2_tap_count  = 1;
                s_sw2_first_tick = now;
            }
        }
    }

    /* Reset tap counter once the window has expired without a 2nd tap */
    if (s_sw2_tap_count == 1 &&
        (now - s_sw2_first_tick) > DOUBLE_TAP_WINDOW_MS)
    {
        s_sw2_tap_count = 0;
    }

    /* ---- SW3 (RIGHT): unassigned — consume event only -------------- */
    (void)buttons_checkButton(RIGHT);

    /* ---- SW4 (LEFT): quiet manual step nudge (IMU fallback) -------- */
    if (buttons_checkButton(LEFT) == PUSHED)
    {
        sc_add_steps(SW4_STEP_NUDGE);
    }
}

bool button_task_is_test_mode(void)
{
    return s_test_mode;
}
