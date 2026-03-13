// *******************************************************
//
// buttons.c
//
//
// Support for a set of FOUR specific buttons on the NUCLEO boards.
// ENCE361 sample code.
// The buttons are UP/DOWN/LEFT/RIGHT.
//
// Created by P.J. Bones, UC ECE
// Updated by Le Yang & F. Youssif, UC ECE.
// Last modified:  15/01/2025
//
// *******************************************************

#include <stdint.h>
#include <stdbool.h>

#include "buttons.h"
#include "stm32c0xx_hal.h"

#define NUM_BUT_POLLS 3

// *******************************************************
// Typedefs
// *******************************************************
typedef struct
{
	// Constant config
	GPIO_TypeDef* const port;
	const uint16_t pin;
	const GPIO_PinState normalState;

	// Runtime properties
	GPIO_PinState state;
	uint8_t newStateCount;
	bool hasChanged;
} buttonProperties_t;

// *******************************************************
// Globals to module
// *******************************************************
buttonProperties_t buttons[NUM_BUTTONS] =
{
		// UP button (SW1, PC11, active HIGH)
		[UP] = {
			.port = GPIOC,
			.pin = GPIO_PIN_11,
			.normalState = GPIO_PIN_RESET
		},
		// DOWN button (SW2, PC1, active HIGH)
		[DOWN] = {
			.port = GPIOC,
			.pin = GPIO_PIN_1,
			.normalState = GPIO_PIN_RESET
		},
	    // LEFT button (SW4, PC13, active LOW)
		[LEFT] =
		{
			.port = GPIOC,
			.pin = GPIO_PIN_13,
			.normalState = GPIO_PIN_SET
		},
		// RIGHT button (SW3, PC10, active HIGH)
		[RIGHT] =
		{
			.port = GPIOC,
			.pin = GPIO_PIN_10,
			.normalState = GPIO_PIN_RESET
		}
};

// *******************************************************
// buttons_init: Initialise the variables associated with the set of buttons.
void buttons_init (void)
{
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		buttons[i].state = buttons[i].normalState;
		buttons[i].newStateCount = 0;
		buttons[i].hasChanged = false;
	}
}

// *******************************************************
// buttons_update: Function designed to be called regularly. It polls all
// buttons once and updates variables associated with the buttons if
// necessary. It is efficient enough to be part of an ISR for e.g.,
// a SysTick interrupt.
// Debouncing algorithm: A finite state machine (FSM) is associated with each button.
// A state change can be declared only after NUM_BUT_POLLS consecutive polls have
// read the pin in the opposite condition, before the state changes and
// a flag is set. Set NUM_BUT_POLLS according to the polling rate.
void buttons_update (void)
{
	// Iterate through the buttons, updating button variables as required
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		// Read the pin value
		GPIO_PinState rawState = HAL_GPIO_ReadPin(buttons[i].port, buttons[i].pin);

		// If reading is different from last confirmed state, increment counter
        if (rawState != buttons[i].state)
        {
        	buttons[i].newStateCount++;

        	// If count exceeds poll count, confirm change of state
        	if (buttons[i].newStateCount >= NUM_BUT_POLLS)
        	{
        		buttons[i].state = rawState;
        		buttons[i].hasChanged = true;	// Reset by call to buttons_checkButton()
        		buttons[i].newStateCount = 0;
        	}
        }
        else
        {
        	buttons[i].newStateCount = 0;
        }
	}
}

// *******************************************************
// buttons_checkButton: Function returns the new button logical state if the button
// logical state (PUSHED or RELEASED) has changed since the last call,
// otherwise returns NO_CHANGE.
buttonState_t buttons_checkButton (buttonName_t butName)
{
	if (buttons[butName].hasChanged)
	{
		buttons[butName].hasChanged = false;
		if (buttons[butName].state == buttons[butName].normalState)
			return RELEASED;
		else
			return PUSHED;
	}
	return NO_CHANGE;
}

