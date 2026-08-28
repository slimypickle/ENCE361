/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc.h
  * @brief   Prototypes and global handles for adc.c
  *
  * ADC1 fixed-sequence channels (ascending channel order):
  *   IN1  (PA1) -> raw_adc[0] : VR1 rotary potentiometer
  *   IN11 (PC4) -> raw_adc[1] : Joystick Y
  *   IN12 (PC5) -> raw_adc[2] : Joystick X
  *
  * Both handles are exposed here so the interrupt vector file and any
  * other module can reach them by including this header instead of
  * redeclaring extern locally.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

void MX_ADC1_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H__ */
