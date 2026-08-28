/*
 * rgb.c
 *
 *  Created on: Dec 5, 2024
 *      Author: fsy13
 */


#include <stdbool.h>

#include "rgb.h"
#include "main.h"
#include "gpio.h"

typedef struct
{
    GPIO_TypeDef* port;
    uint16_t      pin;
    bool          active_high;
} rgb_gpio_config_t;

/* Kept for the colour API. */
static const rgb_gpio_config_t RGB_COLOURS[RGB_NUM_COLOURS] =
{
    [RGB_RED]   = { .port = GPIOD, .pin = GPIO_PIN_3, .active_high = true },
    [RGB_GREEN] = { .port = GPIOD, .pin = GPIO_PIN_2, .active_high = true },
    [RGB_BLUE]  = { .port = GPIOD, .pin = GPIO_PIN_4, .active_high = true },
};


void rgb_led_on    (rgb_led_t led) { (void)led; }
void rgb_led_off   (rgb_led_t led) { (void)led; }
void rgb_led_toggle(rgb_led_t led) { (void)led; }
void rgb_led_all_on (void)         { }
void rgb_led_all_off(void)         { }

/* RGB colour API — drives PD2/PD3/PD4, safe to use                     */

void rgb_colour_on(rgb_colour_t colour)
{
    GPIO_PinState state = RGB_COLOURS[colour].active_high
                          ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(RGB_COLOURS[colour].port,
                      RGB_COLOURS[colour].pin, state);
}

void rgb_colour_off(rgb_colour_t colour)
{
    GPIO_PinState state = RGB_COLOURS[colour].active_high
                          ? GPIO_PIN_RESET : GPIO_PIN_SET;
    HAL_GPIO_WritePin(RGB_COLOURS[colour].port,
                      RGB_COLOURS[colour].pin, state);
}

void rgb_colour_toggle(rgb_colour_t colour)
{
    HAL_GPIO_TogglePin(RGB_COLOURS[colour].port, RGB_COLOURS[colour].pin);
}

void rgb_colour_all_on(void)
{
    for (uint8_t i = 0; i < RGB_NUM_COLOURS; i++) rgb_colour_on(i);
}

void rgb_colour_all_off(void)
{
    for (uint8_t i = 0; i < RGB_NUM_COLOURS; i++) rgb_colour_off(i);
}
