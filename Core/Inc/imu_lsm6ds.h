/*
 * imu_lsm6ds.h
 *
 * Low-level SPI byte driver for the LSM6DS3TR-C IMU.
 * Talks over SPI2 in 16-bit, hardware-NSS, MSB-first mode.
 *
 * Each call is one 16-bit SPI frame:
 *   bit 15    : R/W bit  (1 = read, 0 = write)
 *   bits 14:8 : register address
 *   bits 7:0  : data byte (sent on write, captured on read)
 */

#ifndef INC_IMU_LSM6DS_H_
#define INC_IMU_LSM6DS_H_

#include <stdint.h>

typedef enum {
    /* Embedded-function INT routing / identity */
    INT1_CTRL       = 0x0D,  /* INT1 pin function routing                 */
    WHO_AM_I        = 0x0F,  /* Identity register, fixed value 0x6A       */

    /* Core control */
    CTRL1_XL        = 0x10,  /* Accelerometer ODR / FS / BW               */
    CTRL3_C         = 0x12,  /* BDU, IF_INC, BLE, SW_RESET, ...           */
    CTRL10_C        = 0x19,  /* FUNC_EN / PEDO_EN / PEDO_RST_STEP / ...   */

    /* Accelerometer outputs (signed 16-bit, little-endian L then H) */
    OUTX_L_XL       = 0x28,
    OUTX_H_XL,               /* 0x29 */
    OUTY_L_XL,               /* 0x2A */
    OUTY_H_XL,               /* 0x2B */
    OUTZ_L_XL,               /* 0x2C */
    OUTZ_H_XL,               /* 0x2D */

    /* Step counter outputs (unsigned 16-bit, little-endian L then H) */
    STEP_COUNTER_L  = 0x4B,
    STEP_COUNTER_H,          /* 0x4C */

    /* Embedded-function configuration */
    TAP_CFG         = 0x58,  /* INTERRUPTS_ENABLE, LIR, TAP_x_EN, ...     */
} imu_register_t;

/* ---- Pre-built register bit patterns ------------------------------- */

/* CTRL1_XL = 0xA0 : ODR_XL=1010 (416 Hz), FS_XL=00 (+/-2 g), HM_MODE=0
 * (high-performance).  This is the Lab 8 "high performance" setting.  */
#define CTRL1_XL_HIGH_PERFORMANCE   0xA0U

/* CTRL3_C = 0x44 : BDU=1, IF_INC=1.  Needed for the 2-byte burst read
 * of STEP_COUNTER_L/H to be atomic (BDU) and addressable (IF_INC).    */
#define CTRL3_C_BDU_IFINC           0x44U

/* CTRL10_C = 0x14 : FUNC_EN (bit 2) | PEDO_EN (bit 4).
 * Turns on the embedded functions block and the pedometer algorithm.  */
#define CTRL10_C_PEDO_FUNC_EN       0x14U

/* CTRL10_C bit 1 : PEDO_RST_STEP - set to 1 to zero STEP_COUNTER, then
 * write 0 to release.  The datasheet describes this as a regular control
 * bit (0: disabled, 1: enabled), NOT a self-clearing pulse — the imu_init
 * sequence in imu.c clears it explicitly after a short delay.            */
#define CTRL10_C_PEDO_RST_STEP      0x02U

/* INT1_CTRL = 0x80 : INT1_STEP_DETECTOR - route the step-detected
 * pulse to the INT1 pad (wired to PB10 on the RCAP board).            */
#define INT1_CTRL_STEP_DETECTOR     0x80U

/* TAP_CFG = 0x80 : INTERRUPTS_ENABLE - master enable for embedded
 * function interrupts on the INT pads.  Without this the step
 * detector routing in INT1_CTRL has no electrical effect (AN5040).    */
#define TAP_CFG_INTERRUPTS_ENABLE   0x80U

/* Expected value of WHO_AM_I for the LSM6DS3TR-C. */
#define IMU_WHO_AM_I_VAL            0x6AU

/* ---- Public API (unchanged from Week 8 driver) --------------------- */
void    imu_lsm6ds_write_byte(imu_register_t register_address, uint8_t value);
uint8_t imu_lsm6ds_read_byte (imu_register_t register_address);

#endif /* INC_IMU_LSM6DS_H_ */
