/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32c0xx_it.c
  * @brief   Interrupt service routines.
  *
  *   EXTI4_15  - IMU INT1 on PB10. Each rising edge is one step pulse
  *               from the LSM6DS3TR-C hardware pedometer. The HAL
  *               dispatcher calls our HAL_GPIO_EXTI_Rising_Callback
  *               (in imu.c), which sets a volatile flag picked up by
  *               the main loop.
  *
  *   DMA1_Ch1  - ADC1 conversion buffer.
  *   DMA1_Ch2  - I2C1 TX, drives the OLED writes from ssd1306.c.
  *   I2C1      - OLED transactions, paired with DMA channel 2/3.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "stm32c0xx_it.h"

/* All HAL handles reached via their proper module headers; no local
 * extern declarations needed here. */
#include "adc.h"
#include "i2c.h"

/* ------------------------------------------------------------------ */
/* Cortex-M0+ core handlers                                             */
/* ------------------------------------------------------------------ */
void NMI_Handler(void)
{
    while (1) { }
}

void HardFault_Handler(void)
{
    while (1) { }
}

void SVC_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* ------------------------------------------------------------------ */
/* Peripheral handlers                                                  */
/* ------------------------------------------------------------------ */

/* PB10 / IMU INT1 — dispatched by the HAL. */
void EXTI4_15_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(IMUint1_Pin);
}

/* ADC1 conversion DMA */
void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_adc1);
}

/* I2C1 TX DMA (used by the OLED driver) */
void DMA1_Channel2_3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_i2c1_tx);
}

/* I2C1 event / error. The C0 vector table combines event, error and
 * EXTI line 23 (I2C wakeup) on this IRQ — checking the error flags
 * first means we route obvious bus problems to the error handler. */
void I2C1_IRQHandler(void)
{
    if (hi2c1.Instance->ISR & (I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_OVR))
    {
        HAL_I2C_ER_IRQHandler(&hi2c1);
    }
    else
    {
        HAL_I2C_EV_IRQHandler(&hi2c1);
    }
}
