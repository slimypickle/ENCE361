#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

#include <stdint.h>
#include <stdbool.h>

void button_task_init(void);
void button_task_execute(void);

/* Returns true while test mode is active (toggled by SW2 double-tap) */
bool button_task_is_test_mode(void);

#endif /* BUTTON_TASK_H */
