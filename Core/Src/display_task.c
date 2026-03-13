/*
 * display_task.c
 *
 *  Created on: 12/03/2026
 *      Author: fbu26
 */

#include "display_task.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "stdio.h"
#include "stm32c0xx_hal.h"
#include "usart.h"
// Task period (ms)
#define DISPLAY_TASK_PERIOD_MS 250

// Next run timestamp
static uint32_t nextRun = 0;

// Buffer for formatted string
static char buf[32];

// Pointer to ADC values
// declare the function
extern uint16_t adc_get_value(uint8_t index);

void display_task_init(void)
{
    ssd1306_Init();
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("Hello world!", Font_7x10, White);

    // Initialize next run
    nextRun = HAL_GetTick() + DISPLAY_TASK_PERIOD_MS;
}

void display_task_execute(void)
{
    uint32_t now = HAL_GetTick();
    if (now >= nextRun)
    {
        // Format raw ADC values into the buffer
    	snprintf(buf, sizeof(buf), "X:%4d Y:%4d\r\n", adc_get_value(0), adc_get_value(1));

        // Set cursor to top-left and write string
        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString(buf, Font_7x10, White);

        // Update OLED screen
        ssd1306_UpdateScreen();

        ////send over UART
        HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);

		// Schedule next run
		nextRun += DISPLAY_TASK_PERIOD_MS;

    }
}
