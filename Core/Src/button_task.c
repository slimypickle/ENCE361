/*
 * button_task.c
 *
 *  Created on: 12/03/2026
 *      Author: fbu26
 */
#include "button_task.h"
#include "buttons.h"
#include "rgb.h"
#include "display_task.h"
#include "stm32c0xx_hal.h"
#include "string.h"
#include "tim.h"
#include "pwm.h"
// Task period
#define BUTTON_TASK_PERIOD_MS 20

// Next run timestamp
static uint32_t nextRun = 0;

// Track DS3 duty cycle (0-100% in 10% steps)
static uint8_t duty = 0;

void button_task_init(void)
{
    buttons_init();                  // initialise buttons
    duty = 0;
    pwm_setDutyCycle(&htim2, TIM_CHANNEL_3, duty);  // set initial brightness to 0%
    nextRun = HAL_GetTick() + BUTTON_TASK_PERIOD_MS;
}

void button_task_execute(void)
{
    uint32_t now = HAL_GetTick();
    if (now >= nextRun)
    {
        buttons_update();  // update states

        // SW1 (UP): increase DS3 brightness by 10%, cycle back to 0% after 100% (M1.5)
        if (buttons_checkButton(UP) == PUSHED)
        {
            duty += 10;

            if (duty > 100)
            {
                duty = 0;
            }

            pwm_setDutyCycle(&htim2, TIM_CHANNEL_3, duty);
        }

        // SW2 (DOWN): toggle UART serial debugging (M1.3)
        if (buttons_checkButton(DOWN) == PUSHED)  { display_task_toggle_uart(); }
        if (buttons_checkButton(RIGHT) == PUSHED) { rgb_led_toggle(RGB_RIGHT); rgb_colour_all_on(); }
        if (buttons_checkButton(LEFT) == PUSHED)  { rgb_led_toggle(RGB_LEFT); rgb_colour_all_on(); }

        nextRun += BUTTON_TASK_PERIOD_MS;
    }
}


