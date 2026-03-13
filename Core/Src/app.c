/*
 * app.c
 *
 *  Created on: 26/02/2026
 *      Author: fbu26
 */

#include "app.h"
#include "gpio.h"
#include "rgb.h"
#include "buttons.h"
#include "stm32c0xx_hal.h"  // for HAL_GetTick()
#include "adc.h"
#include "button_task.h"
#include "blinky_task.h"
#include "display_task.h"
#include "stm32c0xx_hal_tim.h"
#include "tim.h"


// ------------------------------
// ADC raw values
// ------------------------------
static uint16_t raw_adc[2];  // keep private

// getter function
uint16_t adc_get_value(uint8_t index)
{
    if (index < 2) return raw_adc[index];
    return 0;
}

// ------------------------------
// HAL ADC conversion complete callback
// ------------------------------
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    // Optional: handle ADC completion here
}

// ------------------------------
// Main application
// ------------------------------
void app_main(void)
{
    // --------------------------
    // Initialise tasks
    // --------------------------
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3); // Start PWM before button task init
    button_task_init();    // Initialise button polling task (sets initial PWM duty to 0%)
    blinky_task_init();    // Initialise LD1 blinking task
    display_task_init();   // Initialises display task


    // --------------------------
    // Main loop (scheduler)
    // --------------------------
    while (1)
    {
        // Execute button task every 20 ms
        button_task_execute();

        // Execute blinky task every 500 ms
        blinky_task_execute();

        // Trigger ADC conversion via DMA
        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)raw_adc, 2);

        // Execute display task every 250ms
        display_task_execute();


    }
}
