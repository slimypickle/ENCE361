/*
 * joystick_task.c
 *
 * Reads joystick ADC channels and computes direction / displacement.
 *
 */

#include "joystick_task.h"
#include "stm32c0xx_hal.h"
#include "app.h"


/* Calibration (measured from physical hardware)                        */

#define X_REST      2168
#define X_MIN        540
#define X_MAX       4030
#define Y_REST      2287
#define Y_MIN        200
#define Y_MAX       4050

/* Deadband in raw ADC counts. The joystick must exceed this distance
 * from its rest position before a direction is registered.
 *
 * Approximate deflection required at the current value of 400:
 *   X right : 400 / (4030 - 2168) = ~21 %
 *   X left  : 400 / (2168 -  540) = ~25 %
 *   Y down  : 400 / (4050 - 2287) = ~23 %
 *   Y up    : 400 / (2287 -  200) = ~19 %
 *
 * The previous value of 200 (≈ 10–12 %) was too sensitive; light
 * contact or ADC noise near the rest position would trigger events.   */
#define DEADBAND     400


/* Task period                                                           */

#define JOYSTICK_TASK_PERIOD_MS  50U



/* Private state                                                        */

static uint32_t          s_next_run      = 0;
static uint16_t          s_x_raw         = 0;
static uint16_t          s_y_raw         = 0;
static uint8_t           s_x_pct         = 0;
static uint8_t           s_y_pct         = 0;
static joystick_x_dir_t  s_x_dir         = JOY_DIR_REST;
static joystick_y_dir_t  s_y_dir         = JOY_DIR_REST_Y;
static const char*       s_x_dir_str     = "Rest ";
static const char*       s_y_dir_str     = "Rest ";

/* Helpers                                                              */

/* Map raw ADC to 0–100 % */
static uint8_t calc_percent(int32_t raw, int32_t rest,
                             int32_t min,  int32_t max)
{
    int32_t diff = raw - rest;
    uint32_t pct;
    if (diff > 0)
        pct = (uint32_t)(diff * 100) / (uint32_t)(max - rest);
    else
        pct = (uint32_t)((-diff) * 100) / (uint32_t)(rest - min);
    return (uint8_t)((pct > 100) ? 100 : pct);
}


/* Public API                                                           */

void joystick_task_init(void)
{
    s_next_run = HAL_GetTick() + JOYSTICK_TASK_PERIOD_MS;
}

void joystick_task_execute(void)
{
    uint32_t now = HAL_GetTick();
    if (now < s_next_run) return;
    s_next_run += JOYSTICK_TASK_PERIOD_MS;

    /*
     * Index 1 = IN11 = PC4 = Y axis
     * Index 2 = IN12 = PC5 = X axis
     * (Index 0 = IN1  = PA1 = VR1, accessed separately through adc_get_vr1)
     */
    s_y_raw = adc_get_value(1);
    s_x_raw = adc_get_value(2);

    int32_t x_diff = (int32_t)s_x_raw - X_REST;
    int32_t y_diff = (int32_t)s_y_raw - Y_REST;

    /* X direction */
    if (x_diff > DEADBAND)
    {
        s_x_dir     = JOY_DIR_LEFT;
        s_x_dir_str = "Left";
        s_x_pct     = calc_percent(s_x_raw, X_REST, X_MIN, X_MAX);
    }
    else if (x_diff < -DEADBAND)
    {
        s_x_dir     = JOY_DIR_RIGHT;
        s_x_dir_str = "Right ";
        s_x_pct     = calc_percent(s_x_raw, X_REST, X_MIN, X_MAX);
    }
    else
    {
        s_x_dir     = JOY_DIR_REST;
        s_x_dir_str = "Rest ";
        s_x_pct     = 0;
    }

    /* Y direction */
    if (y_diff > DEADBAND)
    {
        s_y_dir     = JOY_DIR_DOWN;
        s_y_dir_str = "Down ";
        s_y_pct     = calc_percent(s_y_raw, Y_REST, Y_MIN, Y_MAX);
    }
    else if (y_diff < -DEADBAND)
    {
        s_y_dir     = JOY_DIR_UP;
        s_y_dir_str = "Up   ";
        s_y_pct     = calc_percent(s_y_raw, Y_REST, Y_MIN, Y_MAX);
    }
    else
    {
        s_y_dir     = JOY_DIR_REST_Y;
        s_y_dir_str = "Rest ";
        s_y_pct     = 0;
    }
}

/* ---- Getters ---------------------------------------------------- */
uint16_t          joystick_get_x_raw(void)     { return s_x_raw;     }
uint16_t          joystick_get_y_raw(void)     { return s_y_raw;     }
uint8_t           joystick_get_x_percent(void) { return s_x_pct;     }
uint8_t           joystick_get_y_percent(void) { return s_y_pct;     }
joystick_x_dir_t  joystick_get_x_dir(void)    { return s_x_dir;     }
joystick_y_dir_t  joystick_get_y_dir(void)    { return s_y_dir;     }
const char*       joystick_get_x_dir_str(void){ return s_x_dir_str; }
const char*       joystick_get_y_dir_str(void){ return s_y_dir_str; }
