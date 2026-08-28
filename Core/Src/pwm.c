/*
 * pwm.c
 *
 *  Created on: Dec 18, 2024
 *      Author: fsy13
 *
 * Final-Demo bug fix:
 *   The previous version started the PWM channel only when
 *   HAL_TIM_GetChannelState() reported HAL_TIM_CHANNEL_STATE_READY.
 *   On the STM32C0 HAL shipped with the course image,
 *   HAL_TIM_PWM_ConfigChannel() does NOT transition the channel state
 *   from RESET to READY — the state stays RESET and the conditional
 *   call to HAL_TIM_PWM_Start() is silently skipped.  The CCxE bit in
 *   CCER and the CEN bit in CR1 are never set, so DS3 (driven by
 *   TIM2 CH3) stays dark forever no matter what duty value is written
 *   into CCR3.
 *
 *   The new version writes CCR first, then unconditionally sets CCxE
 *   and CEN via direct register access.  This bypasses the HAL state
 *   machine entirely and is idempotent (re-asserting bits already set
 *   is a no-op), so it's safe to call from led_progress_update() at
 *   250 ms cadence forever.
 */

#include "pwm.h"

void pwm_setDutyCycle(TIM_HandleTypeDef* tim, uint32_t tim_channel, uint8_t duty)
{
    /* Compute and load the new compare value.                            */
    uint32_t reloadValue    = __HAL_TIM_GET_AUTORELOAD(tim);
    uint32_t desiredCompare = duty * (reloadValue / 100U);
    __HAL_TIM_SET_COMPARE(tim, tim_channel, desiredCompare);

    /* Force-enable the channel output (CCxE) regardless of HAL state.
     * Direct register write — idempotent.                                */
    switch (tim_channel)
    {
        case TIM_CHANNEL_1: tim->Instance->CCER |= TIM_CCER_CC1E; break;
        case TIM_CHANNEL_2: tim->Instance->CCER |= TIM_CCER_CC2E; break;
        case TIM_CHANNEL_3: tim->Instance->CCER |= TIM_CCER_CC3E; break;
        case TIM_CHANNEL_4: tim->Instance->CCER |= TIM_CCER_CC4E; break;
        default: break;
    }

    /* Force the timer counter to be running (CEN bit in CR1).            */
    __HAL_TIM_ENABLE(tim);
}

uint8_t pwm_getDutyCycle(TIM_HandleTypeDef* tim, uint32_t tim_channel)
{
    uint32_t reloadValue  = __HAL_TIM_GET_AUTORELOAD(tim);
    uint32_t compareValue = __HAL_TIM_GET_COMPARE(tim, tim_channel);

    /* Protect from division by 0 */
    if (reloadValue == 0U)
    {
        return 0U;
    }

    return (uint8_t)((compareValue * 100U) / reloadValue);
}
