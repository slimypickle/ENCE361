/*
 * ui_task.c
 *
 * User-interface state machine for the ENCE361 step counter.
 *
 * Display states (cycle with joystick left / right):
 *   Steps  →  Distance  →  Goal Progress  →  Steps  (left)
 *   Steps  →  Goal      →  Distance       →  Steps  (right)
 *
 * Joystick UP  (normal mode) : toggle display units
 * Joystick UP  (test mode)   : increment step count proportionally
 * Joystick DOWN(test mode)   : decrement step count proportionally
 *
 * Goal Progress state:
 *   Long-press joystick click (≥ 1 s) → enter Set Goal state
 *
 * Set Goal state:
 *   VR1 potentiometer sets goal value (500–15 000, 100-step increments)
 *   Long-press click (≥ 1 s) → confirm and return to Goal Progress
 *   Short press click         → revert and return to Goal Progress
 *
 * Hardware:
 *   Joystick click : PB1, active-high (PULLDOWN in gpio.c)
 *   VR1 wiper      : PA1 → ADC_IN1  (raw_adc[0] after adding channel)
 */

#include "ui_task.h"
#include "step_counter.h"
#include "buzzer.h"
#include "button_task.h"
#include "joystick_task.h"
#include "app.h"                    /* adc_get_vr1()                      */
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "main.h"
#include "stm32c0xx_hal.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Configuration                                                        */
/* ------------------------------------------------------------------ */
#define TASK_PERIOD_MS          50U
#define DISPLAY_PERIOD_MS       250U

/* Test-mode step rate: 120 steps per update at 100 % displacement
   gives 120 * (1000/50) = 2400 steps/s, reaching 15 000 in ~6.25 s.
   Quadratic scaling ensures delta=1 at minimum joystick push (M2.3d). */
#define TEST_MAX_STEPS_PER_UPDATE   120U

/* Joystick click long-press threshold (ms) */
#define CLICK_LONG_MS           1000U

/* Goal-setting range and granularity */
#define GOAL_MIN                500U
#define GOAL_MAX                15000U
#define GOAL_STEP               100U

/* Dead-zone at the low end of the VR1 potentiometer travel.
 * Physical pots have residual wiper resistance so the ADC never reads 0.
 * Any raw reading at or below this offset is treated as the minimum pot
 * position and maps directly to GOAL_MIN.  The value 300 (≈7 % of 4095)
 * is well above the measured hardware minimum (~141) so the pot reliably
 * reaches 500 even across manufacturing variation.                      */
#define POT_RAW_OFFSET          300U

/* ------------------------------------------------------------------ */
/* Private types                                                        */
/* ------------------------------------------------------------------ */
typedef enum
{
    CLICK_IDLE = 0,
    CLICK_PRESSED_GOAL,     /* held in Goal state, timing for entry      */
    CLICK_HELD_SET_GOAL,    /* entered Set Goal; original press still held*/
    CLICK_PRESSED_CONFIRM,  /* held in Set Goal state, timing confirm     */
} click_sm_t;

/* ------------------------------------------------------------------ */
/* Private state                                                        */
/* ------------------------------------------------------------------ */
static ui_state_t  s_state          = UI_STEPS;
static bool        s_units_alt      = false;   /* false = primary units   */
static bool        s_goal_confirmed = false;

/* Timing */
static uint32_t    s_next_run       = 0;
static uint32_t    s_next_display   = 0;

/* Joystick edge detection (direction must return to Rest before next
   event fires, so rapid continuous deflection doesn't spam events). */
static bool        s_joy_left_fired  = false;
static bool        s_joy_right_fired = false;
static bool        s_joy_up_fired    = false;

/* Click state machine */
static click_sm_t  s_click_sm   = CLICK_IDLE;
static uint32_t    s_click_start = 0;

/* Set-Goal tracking */
static uint32_t    s_prev_goal   = 0;   /* goal before Set Goal entered   */
static uint32_t    s_pending_goal = 0;  /* goal currently shown in Set Goal*/

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

/** Map raw 12-bit ADC value from VR1 to a goal (500–15000 rounded to
 *  the nearest 100 steps).
 *
 *  Any reading at or below POT_RAW_OFFSET maps to GOAL_MIN.  Above the
 *  offset, the usable range [POT_RAW_OFFSET, 4095] is mapped linearly
 *  to [GOAL_MIN, GOAL_MAX], ensuring the physical pot minimum reliably
 *  reaches 500 even with hardware variation.                            */
static uint32_t vr1_to_goal(uint16_t raw)
{
    uint32_t adj = (raw > POT_RAW_OFFSET) ? (uint32_t)(raw - POT_RAW_OFFSET) : 0U;
    uint32_t raw_goal = (adj * (GOAL_MAX - GOAL_MIN)) / (4095U - POT_RAW_OFFSET) + GOAL_MIN;
    /* Round to nearest GOAL_STEP                                        */
    uint32_t rounded = ((raw_goal + GOAL_STEP / 2) / GOAL_STEP) * GOAL_STEP;
    if (rounded < GOAL_MIN) rounded = GOAL_MIN;
    if (rounded > GOAL_MAX) rounded = GOAL_MAX;
    return rounded;
}

/** Advance display state left (Steps→Distance→Goal→Steps). */
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

/** Advance display state right (Steps→Goal→Distance→Steps). */
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

/* ------------------------------------------------------------------ */
/* Click state machine                                                  */
/* ------------------------------------------------------------------ */
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
                /* Clicks in other states are ignored */
            }
            break;

        case CLICK_PRESSED_GOAL:
            if (click == GPIO_PIN_RESET)
            {
                /* Released too soon – not a long press */
                s_click_sm = CLICK_IDLE;
            }
            else if (now - s_click_start >= CLICK_LONG_MS)
            {
                /* Long press detected → enter Set Goal */
                s_prev_goal    = sc_get_goal();
                s_pending_goal = s_prev_goal;
                s_state        = UI_SET_GOAL;
                s_click_sm     = CLICK_HELD_SET_GOAL;
            }
            break;

        case CLICK_HELD_SET_GOAL:
            /* Wait for the user to release the original entry press */
            if (click == GPIO_PIN_RESET)
                s_click_sm = CLICK_IDLE;
            break;

        case CLICK_PRESSED_CONFIRM:
            if (click == GPIO_PIN_RESET)
            {
                uint32_t held = now - s_click_start;
                if (held >= CLICK_LONG_MS)
                {
                    /* Long press → confirm new goal */
                    sc_set_goal(s_pending_goal);
                    s_goal_confirmed = true;
                }
                else
                {
                    /* Short press → revert to previous goal */
                    sc_set_goal(s_prev_goal);
                    s_goal_confirmed = false;
                }
                s_state    = UI_GOAL;
                s_click_sm = CLICK_IDLE;
            }
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Display rendering                                                    */
/* ------------------------------------------------------------------ */
static void render_display(void)
{
    char line1[22], line2[22], line3[22], line4[22];
    bool test_mode = button_task_is_test_mode();

    ssd1306_Fill(Black);

    switch (s_state)
    {
        /* ---- STEPS ------------------------------------------------ */
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
            snprintf(line4, sizeof(line4), test_mode ? "[TEST MODE]" : "");

            ssd1306_SetCursor(0,  0); ssd1306_WriteString(line1, Font_7x10, White);
            ssd1306_SetCursor(0, 14); ssd1306_WriteString(line2, Font_11x18, White);
            ssd1306_SetCursor(0, 36); ssd1306_WriteString(line3, Font_7x10, White);
            ssd1306_SetCursor(0, 50); ssd1306_WriteString(line4, Font_7x10, White);
            break;

        /* ---- DISTANCE --------------------------------------------- */
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

            ssd1306_SetCursor(0,  0); ssd1306_WriteString(line1, Font_7x10, White);
            ssd1306_SetCursor(0, 14); ssd1306_WriteString(line2, Font_11x18, White);
            ssd1306_SetCursor(0, 36); ssd1306_WriteString(line3, Font_7x10, White);
            ssd1306_SetCursor(0, 50); ssd1306_WriteString(line4, Font_7x10, White);
            break;

        /* ---- GOAL PROGRESS ---------------------------------------- */
        case UI_GOAL:
            snprintf(line1, sizeof(line1), "< GOAL: %lu >", (unsigned long)sc_get_goal());
            snprintf(line2, sizeof(line2), "%lu / %lu",
                     (unsigned long)sc_get_steps(),
                     (unsigned long)sc_get_goal());
            snprintf(line3, sizeof(line3), "%lu%% done", (unsigned long)sc_get_percent());
            snprintf(line4, sizeof(line4), test_mode ? "[TEST] Hold=Set" : "Hold click=Set");

            ssd1306_SetCursor(0,  0); ssd1306_WriteString(line1, Font_7x10, White);
            ssd1306_SetCursor(0, 14); ssd1306_WriteString(line2, Font_11x18, White);
            ssd1306_SetCursor(0, 36); ssd1306_WriteString(line3, Font_7x10, White);
            ssd1306_SetCursor(0, 50); ssd1306_WriteString(line4, Font_7x10, White);
            break;

        /* ---- SET GOAL --------------------------------------------- */
        case UI_SET_GOAL:
            /* Update pending goal from potentiometer every render */
            s_pending_goal = vr1_to_goal(adc_get_vr1());

            snprintf(line1, sizeof(line1), "** SET GOAL **");
            snprintf(line2, sizeof(line2), "%lu", (unsigned long)s_pending_goal);
            snprintf(line3, sizeof(line3), "Prev: %lu", (unsigned long)s_prev_goal);
            snprintf(line4, sizeof(line4), "Hold=OK  Short=X");

            ssd1306_SetCursor(0,  0); ssd1306_WriteString(line1, Font_7x10, White);
            ssd1306_SetCursor(0, 14); ssd1306_WriteString(line2, Font_11x18, White);
            ssd1306_SetCursor(0, 36); ssd1306_WriteString(line3, Font_7x10, White);
            ssd1306_SetCursor(0, 50); ssd1306_WriteString(line4, Font_7x10, White);
            break;
    }

    ssd1306_UpdateScreen();
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */
void ui_task_init(void)
{
	ssd1306_Init();
    s_state        = UI_STEPS;
    s_units_alt    = false;
    s_click_sm     = CLICK_IDLE;
    s_next_run     = HAL_GetTick() + TASK_PERIOD_MS;
    s_next_display = HAL_GetTick() + DISPLAY_PERIOD_MS;
}

void ui_task_execute(void)
{
    uint32_t now = HAL_GetTick();
    if (now < s_next_run) return;
    s_next_run += TASK_PERIOD_MS;

    /* ---- Joystick direction and percentage ------------------------- */
    joystick_x_dir_t x_dir = joystick_get_x_dir();
    joystick_y_dir_t y_dir = joystick_get_y_dir();
    uint8_t          y_pct = joystick_get_y_percent();

    /* ---- Joystick click state machine ------------------------------ */
    GPIO_PinState click = HAL_GPIO_ReadPin(Joystick_Click_GPIO_Port,
                                           Joystick_Click_Pin);
    handle_click(click);

    /* ---- Navigation: left / right (only outside Set Goal) ---------- */
    if (s_state != UI_SET_GOAL)
    {
        if (x_dir == JOY_DIR_LEFT && !s_joy_left_fired)
        {
            s_joy_left_fired = true;
            advance_state_left();
        }
        else if (x_dir != JOY_DIR_LEFT)
        {
            s_joy_left_fired = false;
        }

        if (x_dir == JOY_DIR_RIGHT && !s_joy_right_fired)
        {
            s_joy_right_fired = true;
            advance_state_right();
        }
        else if (x_dir != JOY_DIR_RIGHT)
        {
            s_joy_right_fired = false;
        }

        /* ---- Joystick UP ------------------------------------------ */
        if (y_dir == JOY_DIR_UP)
        {
            if (!s_joy_up_fired && !button_task_is_test_mode())
            {
                /* Normal mode: single-edge unit toggle */
                s_joy_up_fired = true;
                s_units_alt    = !s_units_alt;
            }
            /* In test mode the continuous section below handles UP.
               s_joy_up_fired stays false so it does not interfere.    */
        }
        else
        {
            s_joy_up_fired = false;
        }
    }

    /* ---- Test mode: continuous step adjustment --------------------- */
    if (button_task_is_test_mode() && s_state != UI_SET_GOAL)
    {
        if (y_dir == JOY_DIR_UP && y_pct > 0)
        {
            /* Quadratic scaling: delta = y_pct² × MAX / 10000
             * At deadband edge (y_pct ≈ 10): delta = 0 → clamped to 1  (M2.3d)
             * At full deflection (y_pct = 100): delta = MAX             (M2.3c) */
            uint32_t delta = ((uint32_t)y_pct * (uint32_t)y_pct
                              * TEST_MAX_STEPS_PER_UPDATE) / 10000U;
            if (delta < 1) delta = 1;

            /* Clamp: steps must stay ≤ goal - 10  (M2.3e) */
            uint32_t goal     = sc_get_goal();
            uint32_t cur      = sc_get_steps();
            uint32_t max_step = (goal > 10) ? goal - 10 : 0;
            uint32_t new_step = cur + delta;
            if (new_step > max_step) new_step = max_step;
            sc_set_steps(new_step);
        }
        else if (y_dir == JOY_DIR_DOWN && y_pct > 0)
        {
            /* Same quadratic scaling for decrement */
            uint32_t delta = ((uint32_t)y_pct * (uint32_t)y_pct
                              * TEST_MAX_STEPS_PER_UPDATE) / 10000U;
            if (delta < 1) delta = 1;

            int32_t new_step = (int32_t)sc_get_steps() - (int32_t)delta;
            if (new_step < 0) new_step = 0;
            sc_set_steps((uint32_t)new_step);
        }
    }

    /* ---- Goal completion buzzer ------------------------------------ */
    if (sc_goal_newly_reached())
    {
        buzzer_trigger();
    }

    /* ---- Display update at lower rate ------------------------------ */
    if (now >= s_next_display)
    {
        s_next_display += DISPLAY_PERIOD_MS;
        render_display();
    }
}

ui_state_t ui_get_state(void)     { return s_state; }
bool       ui_get_units_alt(void) { return s_units_alt; }
