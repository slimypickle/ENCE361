/*
 * rgb.h
 *
 *  Created on: 26/02/2026
 *      Author: fbu26
 */

#ifndef RGB_H_
#define RGB_H_

/* LED positions */
typedef enum
{
    RGB_LEFT = 0,
    RGB_RIGHT,
    RGB_UP,
    RGB_DOWN,
    RGB_NUM_LEDS
} rgb_led_t;


/* RGB colour channels */
typedef enum
{
    RGB_RED = 0,
    RGB_GREEN,
    RGB_BLUE,
    RGB_NUM_COLOURS
} rgb_colour_t;


/* LED control functions */
void rgb_led_on(rgb_led_t led);
void rgb_led_off(rgb_led_t led);
void rgb_led_toggle(rgb_led_t led);
void rgb_led_all_on(void);
void rgb_led_all_off(void);


/* Colour control functions */
void rgb_colour_on(rgb_colour_t colour);
void rgb_colour_off(rgb_colour_t colour);
void rgb_colour_toggle(rgb_colour_t colour);
void rgb_colour_all_on(void);
void rgb_colour_all_off(void);

#endif /* RGB_H_ */
