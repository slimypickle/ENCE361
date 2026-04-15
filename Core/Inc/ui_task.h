/*
 * ui_task.h
 *
 * User-interface state machine for the step counter.
 *
 * Manages:
 *  - Display states: Steps, Distance, Goal Progress, Set Goal
 *  - Joystick navigation (left / right to cycle states)
 *  - Unit toggle (joystick up in normal mode)
 *  - Test mode step adjustment (joystick up/down when test mode active)
 *  - Joystick-click long-press to enter / confirm / cancel Set Goal
 *  - OLED rendering
 *  - Buzzer trigger on goal completion
 *
 * Task period: 50 ms (display re-render every 250 ms internally).
 */

#ifndef UI_TASK_H_
#define UI_TASK_H_

#include <stdbool.h>

typedef enum
{
    UI_STEPS = 0,
    UI_DISTANCE,
    UI_GOAL,
    UI_SET_GOAL,
} ui_state_t;

void        ui_task_init(void);
void        ui_task_execute(void);

/* Queries for other modules */
ui_state_t  ui_get_state(void);
bool        ui_get_units_alt(void);   /* true = alternate units active     */

#endif /* UI_TASK_H_ */
