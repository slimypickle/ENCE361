/*
 * step_counter.h
 *
 * Data model for step count, goal, and distance.
 * Tracks whether the goal has been newly reached for buzzer alert.
 */

#ifndef STEP_COUNTER_H_
#define STEP_COUNTER_H_

#include <stdint.h>
#include <stdbool.h>

#define SC_DEFAULT_GOAL  1000U

void     sc_init(void);

/* Step count accessors */
void     sc_set_steps(uint32_t n);
void     sc_add_steps(int32_t delta);   /* delta may be negative */
uint32_t sc_get_steps(void);

/* Goal accessors */
void     sc_set_goal(uint32_t new_goal);
uint32_t sc_get_goal(void);

/* Derived values */
uint32_t sc_get_percent(void);          /* 0-100, clamped                  */
float    sc_get_distance_km(void);      /* steps * 0.8 / 1000              */
uint32_t sc_get_distance_yards(void);   /* steps * 0.8 / 0.9144            */

/* Goal completion */
bool     sc_is_goal_reached(void);
bool     sc_goal_newly_reached(void);   /* Returns true exactly once after
                                           goal is first crossed.           */

#endif /* STEP_COUNTER_H_ */
