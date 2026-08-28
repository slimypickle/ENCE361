/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   GPIO initialisation.
  *
  *   PB10 - IMU INT1 (IMUint1 in CubeMX), configured as EXTI line 10
  *          rising-edge input with internal pull-down. The NVIC enable
  *          for EXTI4_15_IRQn is deliberately NOT done here; it is
  *          enabled later by imu_task_init() only after imu_init()
  *          confirms the IMU is present. That way the EXTI handler
  *          can never fire while the IMU subsystem isn't ready.
  *
  *   PD0  - Buzzer (active HIGH via transistor buffer)
  *   PD2/PD3/PD4 - RGB LED (Green/Red/Blue)
  ******************************************************************************
  */
/* USER CODE END Header */

#include "gpio.h"

void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Port clocks */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* Output default levels: DS LEDs off (active-low) */
    HAL_GPIO_WritePin(GPIOC, DS4_Pin | DS2_Pin | LD2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DS1_GPIO_Port, DS1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD,
                      Buzzer_Pin | RGB_Green_Pin | RGB_Red_Pin | RGB_Blue_Pin,
                      GPIO_PIN_RESET);

    /* SW1 / SW2 / SW3 (active HIGH on PC11/PC1/PC10) */
    GPIO_InitStruct.Pin   = SW1_Pin | SW2_Pin | SW3_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* DS4 / DS2 / LD2 outputs */
    GPIO_InitStruct.Pin   = DS4_Pin | DS2_Pin | LD2_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* SW4 (active LOW on PC13) */
    GPIO_InitStruct.Pin   = SW4_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    HAL_GPIO_Init(SW4_GPIO_Port, &GPIO_InitStruct);

    /* DS1 output (PF3) */
    GPIO_InitStruct.Pin   = DS1_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DS1_GPIO_Port, &GPIO_InitStruct);

    /* LD1 (PA5 heartbeat) */
    GPIO_InitStruct.Pin   = LD1_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LD1_GPIO_Port, &GPIO_InitStruct);

    /* Joystick click button (PB1) */
    GPIO_InitStruct.Pin   = Joystick_Click_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    HAL_GPIO_Init(Joystick_Click_GPIO_Port, &GPIO_InitStruct);

    /* IMU INT1 (PB10) — rising-edge EXTI, pull-down so a missing /
     * unpowered IMU keeps the line low instead of floating. NVIC is
     * NOT enabled here; imu_task_init() does that after a successful
     * WHO_AM_I check. */
    GPIO_InitStruct.Pin   = IMUint1_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    HAL_GPIO_Init(IMUint1_GPIO_Port, &GPIO_InitStruct);

    /* Buzzer + RGB LED outputs (Port D) */
    GPIO_InitStruct.Pin   = Buzzer_Pin | RGB_Green_Pin | RGB_Red_Pin | RGB_Blue_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* EXTI4_15 NVIC enable intentionally omitted — see header comment. */
}
