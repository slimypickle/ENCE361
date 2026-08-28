/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   I2C1 driver for the OLED display.
  *
  * I2C1 (SSD1306 OLED, DMA-driven):
  *   PB8 = SCL, PB9 = SDA, AF6.
  *   Pull = GPIO_NOPULL — the OLED module has its own external pull-ups.
  *
  * The IMU is NOT on I2C — it's on SPI2 (see spi.c and imu_lsm6ds.c).
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"

I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_tx;

/* I2C1 init function */
void MX_I2C1_Init(void)
{
  hi2c1.Instance              = I2C1;
  hi2c1.Init.Timing           = 0x00402D41;
  hi2c1.Init.OwnAddress1      = 0;
  hi2c1.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2      = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{
  GPIO_InitTypeDef         GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit   = {0};

  if (i2cHandle->Instance == I2C1)
  {
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInit.I2c1ClockSelection   = RCC_I2C1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin       = OLED_SCL_Pin | OLED_SDA_Pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    __HAL_RCC_I2C1_CLK_ENABLE();

    /* I2C1_TX DMA */
    hdma_i2c1_tx.Instance                 = DMA1_Channel2;
    hdma_i2c1_tx.Init.Request             = DMA_REQUEST_I2C1_TX;
    hdma_i2c1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_i2c1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_i2c1_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_i2c1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_i2c1_tx.Init.Mode                = DMA_NORMAL;
    hdma_i2c1_tx.Init.Priority            = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_i2c1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(i2cHandle, hdmatx, hdma_i2c1_tx);

    HAL_NVIC_SetPriority(I2C1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_IRQn);
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{
  if (i2cHandle->Instance == I2C1)
  {
    __HAL_RCC_I2C1_CLK_DISABLE();
    HAL_GPIO_DeInit(OLED_SCL_GPIO_Port, OLED_SCL_Pin);
    HAL_GPIO_DeInit(OLED_SDA_GPIO_Port, OLED_SDA_Pin);
    HAL_DMA_DeInit(i2cHandle->hdmatx);
    HAL_NVIC_DisableIRQ(I2C1_IRQn);
  }
}
