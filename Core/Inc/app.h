/*
 * app.h
 *
 * Application entry point and shared ADC access.
 */

#ifndef INC_APP_H_
#define INC_APP_H_

#include <stdint.h>

void     app_main(void);

/* Returns a raw 12-bit ADC sample by DMA buffer index.
 *   index 0 → IN1  (PA1) – VR1 potentiometer
 *   index 1 → IN11 (PC4) – Joystick Y
 *   index 2 → IN12 (PC5) – Joystick X           */
uint16_t adc_get_value(uint8_t index);

/* Convenience wrapper for VR1 */
uint16_t adc_get_vr1(void);

#endif /* INC_APP_H_ */
