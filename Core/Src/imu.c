/**
 ******************************************************************************
 * @file    imu.c
 * @brief   LSM6DS3TR-C IMU - interrupt-driven step detection over SPI2.
 *
 * Built on top of the Week 8 lab-provided imu_lsm6ds SPI byte driver
 * (imu_lsm6ds_write_byte / imu_lsm6ds_read_byte over hspi2 @ 16-bit DS).
 *
 * Init sequence (Week 8 sec. 5, cross-checked against AN5040):
 *   1. WHO_AM_I read  -> must equal 0x6A
 *   2. CTRL3_C   = 0x44   BDU=1, IF_INC=1            (coherent burst reads)
 *   3. CTRL1_XL  = 0xA0   416 Hz, +/-2 g, high-perf  (Lab 8 setting)
 *   4. CTRL10_C  = 0x14   FUNC_EN | PEDO_EN          (enable pedometer)
 *   5. INT1_CTRL = 0x80   INT1_STEP_DETECTOR         (route to INT1 pad)
 *   6. TAP_CFG   = 0x80   INTERRUPTS_ENABLE          (master arm)
 *   7. Pulse PEDO_RST_STEP -> guaranteed STEP_COUNTER = 0 at start.
 *
 * Runtime path (interrupt-driven, no periodic polling):
 *   IMU INT1 -> PB10 rising edge -> EXTI line 10 -> EXTI4_15_IRQHandler
 *       -> HAL_GPIO_EXTI_Rising_Callback (below)
 *       -> imu_task_step_isr()           // sets a volatile flag only
 *       ... main loop ...
 *       -> imu_task_execute()
 *       -> imu_read_step_delta()         // 2-byte STEP_COUNTER read
 *       -> sc_add_steps(delta)
 ******************************************************************************
 */

#include "imu.h"
#include "imu_lsm6ds.h"     /* lab-provided SPI byte driver               */
#include "imu_task.h"       /* imu_task_step_isr()                        */
#include "main.h"           /* IMUint1_Pin                                */
#include "stm32c0xx_hal.h"
#include <stddef.h>

/* ----------------------------------------------------------------- */
/* Private state                                                      */
/* ----------------------------------------------------------------- */

static bool     s_imu_available = false;
static uint16_t s_last_count    = 0U;

/* ----------------------------------------------------------------- */
/* Public API                                                         */
/* ----------------------------------------------------------------- */

bool imu_init(void)
{
    /* Power-on settle.  Datasheet boot time is ~35 ms; 100 ms is a
     * comfortable margin and gives the SPI peripheral time to be
     * fully clocked from MX_SPI2_Init().                              */
    HAL_Delay(100U);

    /* ---- 1. WHO_AM_I sanity check -------------------------------- */
    if (imu_lsm6ds_read_byte(WHO_AM_I) != IMU_WHO_AM_I_VAL) {
        s_imu_available = false;
        return false;
    }

    /* ---- 2. CTRL3_C : BDU + IF_INC ------------------------------- */
    imu_lsm6ds_write_byte(CTRL3_C,   CTRL3_C_BDU_IFINC);

    /* ---- 3. CTRL1_XL : 416 Hz, +/-2 g, high-performance ---------- */
    imu_lsm6ds_write_byte(CTRL1_XL,  CTRL1_XL_HIGH_PERFORMANCE);

    /* ---- 4. CTRL10_C : enable embedded functions + pedometer ----- */
    imu_lsm6ds_write_byte(CTRL10_C,  CTRL10_C_PEDO_FUNC_EN);

    /* ---- 5. INT1_CTRL : route step-detector pulse to INT1 -------- */
    imu_lsm6ds_write_byte(INT1_CTRL, INT1_CTRL_STEP_DETECTOR);

    /* ---- 6. TAP_CFG : master arm of embedded-function interrupts - */
    imu_lsm6ds_write_byte(TAP_CFG,   TAP_CFG_INTERRUPTS_ENABLE);

    /* ---- 7. Zero the step counter so we start clean -------------- *
     * PEDO_RST_STEP is self-clearing in hardware, but we rewrite the
     * register without it afterwards to be explicit.                 */
    imu_lsm6ds_write_byte(CTRL10_C,
                          CTRL10_C_PEDO_FUNC_EN | CTRL10_C_PEDO_RST_STEP);
    HAL_Delay(2U);
    imu_lsm6ds_write_byte(CTRL10_C,  CTRL10_C_PEDO_FUNC_EN);

    s_last_count    = 0U;
    s_imu_available = true;
    return true;
}

bool imu_read_step_delta(uint16_t *delta_out)
{
    if (!s_imu_available || delta_out == NULL) {
        return false;
    }

    /* STEP_COUNTER is 16 bits split across registers 0x4B (low) and
     * 0x4C (high).  Two separate single-byte SPI reads.  BDU=1 in
     * CTRL3_C guarantees the value stays coherent between them.      */
    uint8_t lo = imu_lsm6ds_read_byte(STEP_COUNTER_L);
    uint8_t hi = imu_lsm6ds_read_byte(STEP_COUNTER_H);

    uint16_t now = (uint16_t)lo | ((uint16_t)hi << 8U);
    *delta_out   = (uint16_t)(now - s_last_count);   /* wrap-safe */
    s_last_count = now;
    return true;
}

/* ----------------------------------------------------------------- */
/* EXTI rising-edge callback                                          */
/* ----------------------------------------------------------------- */

/**
 * @brief  Fires from EXTI4_15_IRQHandler when PB10 sees a rising edge
 *         from the IMU INT1 line.  ISR-safe: defers all work by simply
 *         setting a volatile flag inside imu_task_step_isr().
 */
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == IMUint1_Pin) {
        imu_task_step_isr();
    }
}
