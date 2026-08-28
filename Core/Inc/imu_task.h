/*
 * imu_task.h
 *
 * Interrupt-driven step counting via the LSM6DS3TR-C IMU.
 *
 */

#ifndef IMU_TASK_H_
#define IMU_TASK_H_

#include <stdbool.h>

/**
 * @brief  Initialise IMU and enable EXTI4_15 NVIC if WHO_AM_I passes.
 *         Must be called BEFORE ui_task_init() to avoid I2C bus conflict.
 */
void imu_task_init(void);

/** @brief No-op poll hook; step counting is fully interrupt-driven. */
void imu_task_execute(void);

/**
 * @brief  Called from HAL_GPIO_EXTI_Rising_Callback for PB10.
 *         ISR-safe: only calls sc_add_steps(1).
 */
void imu_task_step_isr(void);

/** @brief Returns true when the IMU was found and configured successfully. */
bool imu_task_is_available(void);

#endif /* IMU_TASK_H_ */
