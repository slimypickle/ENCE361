/*
 * display_task.h
 *
 * Serial UART debug output task.
 * The OLED display is now managed by ui_task.
 * This module hsa the M1 UART serial output functionality.
 */

#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

void display_task_init(void);
void display_task_execute(void);
void display_task_toggle_uart(void);

#endif /* DISPLAY_TASK_H */
