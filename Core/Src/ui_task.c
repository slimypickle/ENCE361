/*
 * ui_task.c
 *
 * User-interface state machine for the step counter.
 */

#include "ui_task.h"
#include "step_counter.h"
#include "buzzer.h"
#include "button_task.h"
#include "joystick_task.h"
#include "imu_task.h"
#include "app.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "main.h"
#include "stm32c0xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Configuration                                                        */

#define TASK_PERIOD_MS              50U
#define DISPLAY_PERIOD_MS           250U

/* Test-mode rate budget. */
#define TEST_MAX_STEPS_PER_UPDATE   120U
#define TEST_SLOW_THRESHOLD_PCT     30U
#define TEST_SLOW_INTERVAL_MS       300U
#define TEST_DEADZONE_PCT           15U

/* Long-press threshold for the joystick click */
#define CLICK_LONG_MS               1000U

/* Goal-setting range and granularity (Spec UI-4a) */
#define GOAL_MIN                    500U
#define GOAL_MAX                    15000U
#define GOAL_STEP                   100U

/* VR1 dead-band at the bottom of travel */
#define POT_RAW_OFFSET              300U
_Static_assert(POT_RAW_OFFSET < 4095U,
               "POT_RAW_OFFSET must be < 4095 to avoid divide-by-zero");

/* Goal-reached banner duration */
#define GOAL_BANNER_MS              3000U

/* Spec 7e — test mode cap is goal minus this margin */
#define TEST_GOAL_MARGIN            10U


/* Click-button FSM                                                     */
typedef enum
{
    CLICK_IDLE = 0,
    CLICK_PRESSED_GOAL,
    CLICK_HELD_SET_GOAL,
    CLICK_PRESSED_CONFIRM,
} click_sm_t;


/* Private state                                                        */

static ui_state_t  s_state           = UI_STEPS;
static bool        s_units_alt       = false;

static uint32_t    s_next_run        = 0;
static uint32_t    s_next_display    = 0;
static uint32_t    s_test_next_update = 0;

/* Banner */
static bool        s_banner_active   = false;
static uint32_t    s_banner_end_tick = 0;

/* Joystick edge tracking */
static bool        s_joy_left_fired  = false;
static bool        s_joy_right_fired = false;
static bool        s_joy_up_fired    = false;

/* Click FSM */
static click_sm_t  s_click_sm    = CLICK_IDLE;
static uint32_t    s_click_start = 0;

/* Goal-edit transient state */
static uint32_t    s_prev_goal    = 0;
static uint32_t    s_pending_goal = 0;


/* Helpers                                                              */

static uint32_t vr1_to_goal(uint16_t raw)
{
    /* Subtract the low-end dead-band, scale to GOAL_MIN..GOAL_MAX,
     * then snap to the nearest GOAL_STEP. */
    uint32_t adj      = (raw > POT_RAW_OFFSET) ? (uint32_t)(raw - POT_RAW_OFFSET) : 0U;
    uint32_t raw_goal = (adj * (GOAL_MAX - GOAL_MIN)) / (4095U - POT_RAW_OFFSET) + GOAL_MIN;
    uint32_t rounded  = ((raw_goal + GOAL_STEP / 2U) / GOAL_STEP) * GOAL_STEP;
    if (rounded < GOAL_MIN) rounded = GOAL_MIN;
    if (rounded > GOAL_MAX) rounded = GOAL_MAX;
    return rounded;
}

/* Spec UI-1: STEPS -> DISTANCE -> GOAL -> STEPS */
static void advance_state_left(void)
{
    switch (s_state)
    {
        case UI_STEPS:    s_state = UI_DISTANCE; break;
        case UI_DISTANCE: s_state = UI_GOAL;     break;
        case UI_GOAL:     s_state = UI_STEPS;    break;
        default: break;
    }
}

/* Spec UI-2: STEPS -> GOAL -> DISTANCE -> STEPS */
static void advance_state_right(void)
{
    switch (s_state)
    {
        case UI_STEPS:    s_state = UI_GOAL;     break;
        case UI_GOAL:     s_state = UI_DISTANCE; break;
        case UI_DISTANCE: s_state = UI_STEPS;    break;
        default: break;
    }
}


static void handle_click(GPIO_PinState click)
{
    uint32_t now = HAL_GetTick();

    switch (s_click_sm)
    {
        case CLICK_IDLE:
            if (click == GPIO_PIN_SET)
            {
                s_click_start = now;
                if (s_state == UI_GOAL)
                    s_click_sm = CLICK_PRESSED_GOAL;
                else if (s_state == UI_SET_GOAL)
                    s_click_sm = CLICK_PRESSED_CONFIRM;
                /* any other state -> click is ignored */
            }
            break;

        case CLICK_PRESSED_GOAL:
            if (click == GPIO_PIN_RESET)
            {
                /* Released before 1 s — not a long press, do nothing. */
                s_click_sm = CLICK_IDLE;
            }
            else if (now - s_click_start >= CLICK_LONG_MS)
            {
                /* Held long enough — drop into Set Goal. */
                s_prev_goal    = sc_get_goal();
                s_pending_goal = s_prev_goal;
                s_state        = UI_SET_GOAL;
                s_click_sm     = CLICK_HELD_SET_GOAL;
            }
            break;

        case CLICK_HELD_SET_GOAL:
            /* Wait for the user to let go of the long-press that
             * brought us here, otherwise the same press would
             * immediately trigger CLICK_PRESSED_CONFIRM. */
            if (click == GPIO_PIN_RESET) s_click_sm = CLICK_IDLE;
            break;

        case CLICK_PRESSED_CONFIRM:
            /* In UI_SET_GOAL — long release confirms, short release
             * cancels (Spec UI-4b). The goal isn't written until
             * confirm, so cancel needs no rollback. */
            if (click == GPIO_PIN_RESET)
            {
                uint32_t held = now - s_click_start;
                if (held >= CLICK_LONG_MS)
                {
                    sc_set_goal(s_pending_goal);
                }
                /* Cancel: do nothing — leave the old goal intact.
                 * We deliberately do NOT call sc_set_goal(s_prev_goal)
                 * here because that would reset the "newly reached"
                 * latch and could swallow a buzzer event. */

                s_state    = UI_GOAL;
                s_click_sm = CLICK_IDLE;
            }
            break;
    }
}


/* Display rendering                                                    */


/* Full-screen banner shown for GOAL_BANNER_MS after the goal is hit. */
static void render_goal_banner(void)
{
    ssd1306_Fill(Black);
    ssd1306_SetCursor(4,  2);   ssd1306_WriteString("GOAL",    Font_11x18, White);
    ssd1306_SetCursor(0, 24);   ssd1306_WriteString("REACHED", Font_11x18, White);
    ssd1306_SetCursor(16, 46);  ssd1306_WriteString("!!!!!",   Font_11x18, White);
    ssd1306_UpdateScreen();
}

static void render_display(void)
{
    if (s_banner_active)
    {
        render_goal_banner();
        return;
    }

    char line1[22], line2[22], line3[22], line4[22];
    bool test_mode = button_task_is_test_mode();

    ssd1306_Fill(Black);

    switch (s_state)
    {
        case UI_STEPS:
            if (!s_units_alt)
            {
                snprintf(line1, sizeof(line1), "< STEPS >");
                snprintf(line2, sizeof(line2), "%lu", (unsigned long)sc_get_steps());
                snprintf(line3, sizeof(line3), "Goal: %lu", (unsigned long)sc_get_goal());
            }
            else
            {
                snprintf(line1, sizeof(line1), "< STEPS %% >");
                snprintf(line2, sizeof(line2), "%lu%%", (unsigned long)sc_get_percent());
                snprintf(line3, sizeof(line3), "Goal: %lu", (unsigned long)sc_get_goal());
            }

            /* Show the active input source so the marker can see
             * whether the IMU pedometer is live. */
            if (imu_task_is_available())
                snprintf(line4, sizeof(line4), test_mode ? "[IMU][TEST]" : "[IMU]");
            else
                snprintf(line4, sizeof(line4), test_mode ? "[BTN][TEST]" : "[BTN]");

            ssd1306_SetCursor(0,  0); ssd1306_WriteString(line1, Font_7x10,  White);
            ssd1306_SetCursor(0, 14); ssd1306_WriteString(line2, Font_11x18, White);
            ssd1306_SetCursor(0, 36); ssd1306_WriteString(line3, Font_7x10,  White);
            ssd1306_SetCursor(0, 50); ssd1306_WriteString(line4, Font_7x10,  White);
            break;

        case UI_DISTANCE:
            if (!s_units_alt)
            {
                float    km       = sc_get_distance_km();
                uint32_t km_whole = (uint32_t)km;
                uint32_t km_dec   = (uint32_t)((km - (float)km_whole) * 100.0f + 0.5f);
                if (km_dec >= 100) { km_whole++; km_dec = 0; }
                snprintf(line1, sizeof(line1), "< DISTANCE >");
                snprintf(line2, sizeof(line2), "%lu.%02lu km",
                         (unsigned long)km_whole, (unsigned long)km_dec);
            }
            else
            {
                uint32_t yards = sc_get_distance_yards();
                snprintf(line1, sizeof(line1), "< DISTANCE >");
                snprintf(line2, sizeof(line2), "%lu yd", (unsigned long)yards);
            }
            snprintf(line3, sizeof(line3), "Steps: %lu", (unsigned long)sc_get_steps());
            snprintf(line4, sizeof(line4), test_mode ? "[TEST MODE]" : "");

            ssd1306_SetCursor(0,  0); ssd1306_WriteString(line1, Font_7x10,  White);
            ssd1306_SetCursor(0, 14); ssd1306_WriteString(line2, Font_11x18, White);
            ssd1306_SetCursor(0, 36); ssd1306_WriteString(line3, Font_7x10,  White);
            ssd1306_SetCursor(0, 50); ssd1306_WriteString(line4, Font_7x10,  White);
            break;

        case UI_GOAL:
            snprintf(line1, sizeof(line1), "< GOAL: %lu >", (unsigned long)sc_get_goal());
            snprintf(line2, sizeof(line2), "%lu / %lu",
                     (unsigned long)sc_get_steps(),
                     (unsigned long)sc_get_goal());
            snprintf(line3, sizeof(line3), "%lu%% done", (unsigned long)sc_get_percent());
            snprintf(line4, sizeof(line4), test_mode ? "[TEST] Hold=Set" : "Hold click=Set");

            ssd1306_SetCursor(0,  0); ssd1306_WriteString(line1, Font_7x10,  White);
            ssd1306_SetCursor(0, 14); ssd1306_WriteString(line2, Font_11x18, White);
            ssd1306_SetCursor(0, 36); ssd1306_WriteString(line3, Font_7x10,  White);
            ssd1306_SetCursor(0, 50); ssd1306_WriteString(line4, Font_7x10,  White);
            break;

        case UI_SET_GOAL:
            /* Live preview from VR1 — written to s_pending_goal so a
             * subsequent confirm picks up the current value. */
            s_pending_goal = vr1_to_goal(adc_get_vr1());

            snprintf(line1, sizeof(line1), "** SET GOAL **");
            snprintf(line2, sizeof(line2), "%lu", (unsigned long)s_pending_goal);
            snprintf(line3, sizeof(line3), "Prev: %lu", (unsigned long)s_prev_goal);
            snprintf(line4, sizeof(line4), "Hold=OK  Short=X");

            ssd1306_SetCursor(0,  0); ssd1306_WriteString(line1, Font_7x10,  White);
            ssd1306_SetCursor(0, 14); ssd1306_WriteString(line2, Font_11x18, White);
            ssd1306_SetCursor(0, 36); ssd1306_WriteString(line3, Font_7x10,  White);
            ssd1306_SetCursor(0, 50); ssd1306_WriteString(line4, Font_7x10,  White);
            break;
    }

    ssd1306_UpdateScreen();
}


/* Public API */

void ui_task_init(void)
{
    ssd1306_Init();
    s_state             = UI_STEPS;
    s_units_alt         = false;
    s_click_sm          = CLICK_IDLE;
    s_banner_active     = false;
    s_joy_left_fired    = false;
    s_joy_right_fired   = false;
    s_joy_up_fired      = false;
    s_test_next_update  = 0;
    s_next_run          = HAL_GetTick() + TASK_PERIOD_MS;
    s_next_display      = HAL_GetTick() + DISPLAY_PERIOD_MS;
}

void ui_task_execute(void)
{
    uint32_t now = HAL_GetTick();
    if (now < s_next_run) return;
    s_next_run += TASK_PERIOD_MS;

    /* Expire the goal-reached banner */
    if (s_banner_active && now >= s_banner_end_tick)
    {
        s_banner_active = false;
    }

    joystick_x_dir_t x_dir = joystick_get_x_dir();
    joystick_y_dir_t y_dir = joystick_get_y_dir();
    uint8_t          y_pct = joystick_get_y_percent();
    bool             test_mode = button_task_is_test_mode();

    /* Click button FSM (handles Goal -> Set Goal -> confirm/cancel) */
    GPIO_PinState click = HAL_GPIO_ReadPin(Joystick_Click_GPIO_Port,
                                           Joystick_Click_Pin);
    handle_click(click);

    /*Joystick edge detection */
    if (x_dir == JOY_DIR_LEFT)
    {
        if (!s_joy_left_fired)
        {
            s_joy_left_fired = true;
            if (s_state != UI_SET_GOAL) advance_state_left();
        }
    }
    else
    {
        s_joy_left_fired = false;
    }

    if (x_dir == JOY_DIR_RIGHT)
    {
        if (!s_joy_right_fired)
        {
            s_joy_right_fired = true;
            if (s_state != UI_SET_GOAL) advance_state_right();
        }
    }
    else
    {
        s_joy_right_fired = false;
    }

    if (y_dir == JOY_DIR_UP)
    {
        if (!s_joy_up_fired)
        {
            s_joy_up_fired = true;
            /* UP toggles units (Spec UI-3), unless test mode is
             * active (UP increments steps) or we're in Set Goal
             * (input ignored). */
            if (s_state != UI_SET_GOAL && !test_mode)
            {
                s_units_alt = !s_units_alt;
            }
        }
    }
    else
    {
        s_joy_up_fired = false;
    }

    /* Test mode: continuous step adjustment */
    if (test_mode && s_state != UI_SET_GOAL)
    {
        if ((y_dir == JOY_DIR_UP || y_dir == JOY_DIR_DOWN) &&
            y_pct >= TEST_DEADZONE_PCT)
        {
            /* Rescale the post-deadband percentage */
            uint32_t shifted = (uint32_t)y_pct - TEST_DEADZONE_PCT;
            uint32_t scaled  = (shifted * 100U) / (100U - TEST_DEADZONE_PCT);
            uint32_t delta   = (scaled * scaled * TEST_MAX_STEPS_PER_UPDATE) / 10000U;
            if (delta < 1U) delta = 1U;

            /* Slow-tick when near the deadzone so single increments
             * are achievable (Spec 7d). */
            uint32_t interval = (y_pct < TEST_SLOW_THRESHOLD_PCT)
                                ? TEST_SLOW_INTERVAL_MS
                                : TASK_PERIOD_MS;

            if (now >= s_test_next_update)
            {
                s_test_next_update = now + interval;

                if (y_dir == JOY_DIR_UP)
                {
                    /* Spec 7e: clamp at goal-10. Critically, only
                     * add when below the cap — never subtract when
                     * the user is pushing UP. */
                    uint32_t goal     = sc_get_goal();
                    uint32_t cur      = sc_get_steps();
                    uint32_t max_step = (goal > TEST_GOAL_MARGIN)
                                        ? goal - TEST_GOAL_MARGIN
                                        : 0U;

                    if (cur < max_step)
                    {
                        uint32_t new_step = cur + delta;
                        if (new_step > max_step) new_step = max_step;
                        sc_set_steps(new_step);
                    }
                }
                else /* JOY_DIR_DOWN */
                {
                    /* Spec 7e: floor at zero. */
                    int32_t new_step = (int32_t)sc_get_steps() - (int32_t)delta;
                    if (new_step < 0) new_step = 0;
                    sc_set_steps((uint32_t)new_step);
                }
            }
        }
        else
        {
            /* Inside the deadzone — reset the schedule so the next
             * deflection fires immediately rather than waiting out
             * a leftover interval. */
            s_test_next_update = 0;
        }
    }

    /* Goal completion (Spec 6 + UI-6) */
    if (sc_goal_newly_reached())
    {
        buzzer_trigger();
        s_banner_active   = true;
        s_banner_end_tick = now + GOAL_BANNER_MS;
    }

    /* OLED redraw at the slower display rate */
    if (now >= s_next_display)
    {
        s_next_display += DISPLAY_PERIOD_MS;
        render_display();
    }
}

ui_state_t ui_get_state(void)     { return s_state; }
bool       ui_get_units_alt(void) { return s_units_alt; }
