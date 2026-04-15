/*
 * joystick_task.h
 *
 * Reads joystick ADC channels, computes direction and percentage
 * displacement for each axis.
 *
 * ADC channel order (ADC_SCAN_SEQ_FIXED, ascending channel number):
 *   raw_adc[0] = IN1  → VR1 potentiometer (PA1)    — managed by app.c
 *   raw_adc[1] = IN11 → Joystick Y axis   (PC4)
 *   raw_adc[2] = IN12 → Joystick X axis   (PC5)
 */

#ifndef JOYSTICK_TASK_H
#define JOYSTICK_TASK_H

#include <stdint.h>

/* Direction enumerations for clean inter-module comparisons */
typedef enum
{
    JOY_DIR_REST  = 0,
    JOY_DIR_LEFT,
    JOY_DIR_RIGHT
} joystick_x_dir_t;

typedef enum
{
    JOY_DIR_REST_Y = 0,
    JOY_DIR_UP,
    JOY_DIR_DOWN
} joystick_y_dir_t;

void              joystick_task_init(void);
void              joystick_task_execute(void);

/* Raw 12-bit ADC values */
uint16_t          joystick_get_x_raw(void);
uint16_t          joystick_get_y_raw(void);

/* 0–100 % displacement (0 inside dead-band) */
uint8_t           joystick_get_x_percent(void);
uint8_t           joystick_get_y_percent(void);

/* Enum-based direction accessors */
joystick_x_dir_t  joystick_get_x_dir(void);
joystick_y_dir_t  joystick_get_y_dir(void);

/* Legacy string accessors (kept for display_task UART output) */
const char*       joystick_get_x_dir_str(void);
const char*       joystick_get_y_dir_str(void);

#endif /* JOYSTICK_TASK_H */
