/*
 * led_progress.h
 *
 * Goal-progress LED bar using DS1–DS4 on the RCAP board.
 *
 * DS3 (PC6 / TIM2 CH3 PWM) brightens from 0→100% over the first 25% of goal.
 * DS4 (PC12) turns solid on at 25%, DS2 (PC2) at 50%, DS1 (PF3) at 75%.
 */

#ifndef LED_PROGRESS_H_
#define LED_PROGRESS_H_

/** @brief Initialise LED bar — all LEDs off, DS3 PWM at 0%. */
void led_progress_init(void);

/**
 * @brief  Update LED bar to reflect current step-count percentage.
 *         Rate-limited internally to 250 ms; safe to call every loop.
 */
void led_progress_update(void);

#endif /* LED_PROGRESS_H_ */
