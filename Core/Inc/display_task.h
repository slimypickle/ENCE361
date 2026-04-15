/*
 * display_task.h
 *
 * Serial UART debug output task.
 * The OLED display is now fully managed by ui_task.
 * This module retains the M1.2/M1.3 UART serial output functionality.
 */

#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

void display_task_init(void);
void display_task_execute(void);
void display_task_toggle_uart(void);

#endif /* DISPLAY_TASK_H */
