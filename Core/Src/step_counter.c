/*
 * step_counter.c
 *
 * Data model for step count, goal, and distance.
 *
 * One step is assumed to be 0.8 m (per project specification).
 */

#include "step_counter.h"

/* ------------------------------------------------------------------ */
/* Private state                                                        */
/* ------------------------------------------------------------------ */
static uint32_t s_steps             = 0;
static uint32_t s_goal              = SC_DEFAULT_GOAL;
static bool     s_prev_reached      = false;
static bool     s_newly_reached     = false;

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */
static void update_reached_flags(void)
{
    bool reached = (s_steps >= s_goal);
    if (reached && !s_prev_reached)
    {
        s_newly_reached = true;
    }
    s_prev_reached = reached;
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */
void sc_init(void)
{
    s_steps         = 0;
    s_goal          = SC_DEFAULT_GOAL;
    s_prev_reached  = false;
    s_newly_reached = false;
}

void sc_set_steps(uint32_t n)
{
    s_steps = n;
    update_reached_flags();
}

void sc_add_steps(int32_t delta)
{
    int32_t result = (int32_t)s_steps + delta;
    s_steps = (result < 0) ? 0 : (uint32_t)result;
    update_reached_flags();
}

uint32_t sc_get_steps(void)  { return s_steps; }

void sc_set_goal(uint32_t new_goal)
{
    s_goal         = new_goal;
    /* Re-synchronise reached state to new goal; do NOT fire newly_reached
       just because the goal was lowered below current steps.              */
    s_prev_reached = (s_steps >= s_goal);
}

uint32_t sc_get_goal(void) { return s_goal; }

uint32_t sc_get_percent(void)
{
    if (s_goal == 0) return 100;
    uint32_t pct = (s_steps * 100U) / s_goal;
    return (pct > 100) ? 100 : pct;
}

/* 1 step = 0.8 m = 0.0008 km */
float sc_get_distance_km(void)
{
    return (float)s_steps * 0.0008f;
}

/* 1 yard = 0.9144 m  →  distance_m / 0.9144 */
uint32_t sc_get_distance_yards(void)
{
    float dist_m = (float)s_steps * 0.8f;
    return (uint32_t)(dist_m / 0.9144f);
}

bool sc_is_goal_reached(void)
{
    return s_steps >= s_goal;
}

bool sc_goal_newly_reached(void)
{
    if (s_newly_reached)
    {
        s_newly_reached = false;
        return true;
    }
    return false;
}
