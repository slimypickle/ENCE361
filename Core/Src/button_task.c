/*
 * button_task.c
 *
 *  Created on: 12/03/2026
 *      Author: fbu26
 */
#include "button_task.h"
#include "buttons.h"
#include "rgb.h"
#include "stm32c0xx_hal.h"
#include "string.h"
#include "tim.h"
#include "pwm.h"
// Task period
#define BUTTON_TASK_PERIOD_MS 20

// Next run timestamp
static uint32_t nextRun = 0;

// Track DS3 duty cycle
static uint8_t duty = 0;

void button_task_init(void)
{
    buttons_init();                  // initialise buttons
    nextRun = HAL_GetTick() + BUTTON_TASK_PERIOD_MS;
}

void button_task_execute(void)
{
    uint32_t now = HAL_GetTick();
    if (now >= nextRun)
    {
        buttons_update();  // update states

        // check each button
        if (buttons_checkButton(UP) == PUSHED)
                {
                    duty += 25;

                    if (duty > 100)
                    {
                        duty = 0;
                    }

                    pwm_setDutyCycle(&htim2, TIM_CHANNEL_3, duty);
                }
        if (buttons_checkButton(DOWN) == PUSHED)  { rgb_led_toggle(RGB_DOWN); rgb_colour_all_on(); }
        if (buttons_checkButton(RIGHT) == PUSHED) { rgb_led_toggle(RGB_RIGHT);rgb_colour_all_on(); }
        if (buttons_checkButton(LEFT) == PUSHED)  { rgb_led_toggle(RGB_LEFT); rgb_colour_all_on(); }

        nextRun += BUTTON_TASK_PERIOD_MS;
    }
}

