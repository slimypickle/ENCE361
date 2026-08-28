/*
 * led_progress.c
 *
 * Goal-progress LED bar
 */

#include "led_progress.h"
#include "step_counter.h"
#include "pwm.h"
#include "tim.h"
#include "main.h"
#include "stm32c0xx_hal.h"

/* Update period — same as the OLED redraw rate */
#define LED_PROGRESS_PERIOD_MS  250U

/* DS4, DS2, DS1 are active-LOW (cathode driven by GPIO) */
#define LED_ON   GPIO_PIN_RESET
#define LED_OFF  GPIO_PIN_SET

/* Self-test step duration  */
#define SELF_TEST_STEP_MS       80U

static uint32_t s_next_run = 0U;


/* Helpers                                                              */

static void all_leds_off(void)
{
    pwm_setDutyCycle(&htim2, TIM_CHANNEL_3, 0U);
    HAL_GPIO_WritePin(DS4_GPIO_Port, DS4_Pin, LED_OFF);
    HAL_GPIO_WritePin(DS2_GPIO_Port, DS2_Pin, LED_OFF);
    HAL_GPIO_WritePin(DS1_GPIO_Port, DS1_Pin, LED_OFF);
}

/* Public API                                                           */

void led_progress_init(void)
{
    /* Properly start the PWM channel via the HAL. */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);

    /* Start from a known-off state */
    all_leds_off();


     /* Flash each LED one at a time, then all four together. */

    /* DS3 first — if this stays dark, the PWM channel isn't running. */
    pwm_setDutyCycle(&htim2, TIM_CHANNEL_3, 100U);
    HAL_Delay(SELF_TEST_STEP_MS);
    pwm_setDutyCycle(&htim2, TIM_CHANNEL_3, 0U);

    HAL_GPIO_WritePin(DS4_GPIO_Port, DS4_Pin, LED_ON);
    HAL_Delay(SELF_TEST_STEP_MS);
    HAL_GPIO_WritePin(DS4_GPIO_Port, DS4_Pin, LED_OFF);

    HAL_GPIO_WritePin(DS2_GPIO_Port, DS2_Pin, LED_ON);
    HAL_Delay(SELF_TEST_STEP_MS);
    HAL_GPIO_WritePin(DS2_GPIO_Port, DS2_Pin, LED_OFF);

    HAL_GPIO_WritePin(DS1_GPIO_Port, DS1_Pin, LED_ON);
    HAL_Delay(SELF_TEST_STEP_MS);
    HAL_GPIO_WritePin(DS1_GPIO_Port, DS1_Pin, LED_OFF);

    /* All four at once — sanity-check the rail can drive them together. */
    pwm_setDutyCycle(&htim2, TIM_CHANNEL_3, 100U);
    HAL_GPIO_WritePin(DS4_GPIO_Port, DS4_Pin, LED_ON);
    HAL_GPIO_WritePin(DS2_GPIO_Port, DS2_Pin, LED_ON);
    HAL_GPIO_WritePin(DS1_GPIO_Port, DS1_Pin, LED_ON);
    HAL_Delay(SELF_TEST_STEP_MS);

    /* End clean */
    all_leds_off();

    s_next_run = HAL_GetTick() + LED_PROGRESS_PERIOD_MS;
}

void led_progress_update(void)
{
    /* Rate-limit to 250 ms — the OLED redraws at the same rate so the
     * LED bar and the displayed percentage step together. */
    uint32_t now = HAL_GetTick();
    if (now < s_next_run) return;
    s_next_run += LED_PROGRESS_PERIOD_MS;

    uint32_t pct = sc_get_percent();   /* already clamped 0..100 */

    /* DS3: PWM ramp 0..100 % across the first quartile */
    if (pct < 25U)
    {
        pwm_setDutyCycle(&htim2, TIM_CHANNEL_3, (uint8_t)(pct * 4U));
    }
    else
    {
        pwm_setDutyCycle(&htim2, TIM_CHANNEL_3, 100U);
    }

    /* DS4 / DS2 / DS1 fill in at 50 / 75 / 100 % */
    HAL_GPIO_WritePin(DS4_GPIO_Port, DS4_Pin,
                      (pct >=  50U) ? LED_ON : LED_OFF);
    HAL_GPIO_WritePin(DS2_GPIO_Port, DS2_Pin,
                      (pct >=  75U) ? LED_ON : LED_OFF);
    HAL_GPIO_WritePin(DS1_GPIO_Port, DS1_Pin,
                      (pct >= 100U) ? LED_ON : LED_OFF);
}
