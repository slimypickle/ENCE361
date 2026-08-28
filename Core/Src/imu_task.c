/*
 * imu_task.c
 *
 * Interrupt-driven step counting on the LSM6DS3TR-C.
 *
 */

#include "imu_task.h"
#include "imu.h"
#include "step_counter.h"
#include "stm32c0xx_hal.h"


/* Private state                                                        */

static bool s_imu_available = false;

/* Written from ISR context, read from the main loop. volatile so the
 * compiler can't cache the read in a register across the ISR. */
static volatile bool s_step_pending = false;


/* Public API                                                           */

void imu_task_init(void)
{
    s_step_pending  = false;
    s_imu_available = imu_init();

    if (s_imu_available)
    {
        HAL_NVIC_SetPriority(EXTI4_15_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
    }
}

void imu_task_execute(void)
{
    if (!s_imu_available) return;
    if (!s_step_pending)  return;

    /* Snapshot-and-clear the flag *before* the SPI read. */
    s_step_pending = false;

    uint16_t delta = 0U;
    if (imu_read_step_delta(&delta) && delta != 0U)
    {
        sc_add_steps((int32_t)delta);
    }
}

/*imu_task_step_isr — called from the EXTI rising-edge callback. */
void imu_task_step_isr(void)
{
    s_step_pending = true;
}

bool imu_task_is_available(void)
{
    return s_imu_available;
}
