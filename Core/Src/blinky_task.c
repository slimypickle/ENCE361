/*
 * blinky_task.c
 *
 *  Created on: 12/03/2026
 *      Author: fbu26
 */
#include "blinky_task.h"
#include "gpio.h"
#include "stm32c0xx_hal.h"

// Task period
#define BLINKY_TASK_PERIOD_MS 500

// Next run timestamp
static uint32_t nextRun = 0;

void blinky_task_init(void)
{
    nextRun = HAL_GetTick() + BLINKY_TASK_PERIOD_MS;
}

void blinky_task_execute(void)
{
    uint32_t now = HAL_GetTick();
    if (now >= nextRun)
    {
        HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
        nextRun += BLINKY_TASK_PERIOD_MS;
    }
}

