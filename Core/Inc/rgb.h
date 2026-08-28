/*
 * rgb.h
 *
 *  Created on: Dec 5, 2024
 *      Author: fsy13
 */

#ifndef INC_RGB_H_
#define INC_RGB_H_

typedef enum
{
	RGB_LEFT,
	RGB_RIGHT,
	RGB_UP,
	RGB_DOWN,
	RGB_NUM_LEDS
} rgb_led_t;


typedef enum
{
	RGB_RED,
	RGB_GREEN,
	RGB_BLUE,
	RGB_NUM_COLOURS
} rgb_colour_t;

void rgb_led_on(rgb_led_t);
void rgb_colour_on(rgb_colour_t);
void rgb_led_off(rgb_led_t);
void rgb_colour_off(rgb_colour_t);
void rgb_led_toggle(rgb_led_t led);
void rgb_colour_toggle(rgb_colour_t colour);
void rgb_led_all_on(void);
void rgb_colour_all_on(void);
void rgb_led_all_off(void);
void rgb_colour_all_off(void);

#endif /* INC_RGB_H_ */
