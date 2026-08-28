/*
 * imu.h
 *
 * App level driver for the LSM6DS3TR-C IMU on the RCAP board.
 * Built on top of the lab-provided imu_lsm6ds SPI byte driver.
 *
 */

#ifndef IMU_H_
#define IMU_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Configure the LSM6DS3TR-C for interrupt-driven step detection.
 * @retval true   IMU found (WHO_AM_I = 0x6A) and all registers programmed.
 * @retval false  WHO_AM_I mismatch - caller falls back to SW4.
 */
bool imu_init(void);

/**
 * @brief  Read the IMU's 16-bit on-chip step counter and return the
 *         number of new steps since the previous successful call.
 *         Handles uint16_t wrap correctly via modular subtraction.
 * @param  delta_out  Number of new steps since the previous call.
 * @retval true on success, false if the IMU is absent or arg is NULL.
 */
bool imu_read_step_delta(uint16_t *delta_out);

#endif /* IMU_H_ */
