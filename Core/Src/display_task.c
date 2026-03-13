/*
 * display_task.c
 *
 *  Created on: 12/03/2026
 *      Author: fbu26
 */

#include "display_task.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "stm32c0xx_hal.h"
#include "usart.h"

// Task period (ms)
#define DISPLAY_TASK_PERIOD_MS 250

// ADC midpoint and deadzone for joystick processing
#define ADC_CENTER   2048
#define ADC_DEADZONE 150

// Next run timestamp
static uint32_t nextRun = 0;

// UART serial debug toggle (enabled by default)
static bool uart_enabled = true;

// Buffers for OLED lines and UART
static char buf_x[20];
static char buf_y[20];
static char uart_buf[48];

// Pointer to ADC values
extern uint16_t adc_get_value(uint8_t index);

// Toggle UART serial debugging on/off (M1.3)
void display_task_toggle_uart(void)
{
    uart_enabled = !uart_enabled;
}

// Process a raw ADC value into a direction string and percentage displacement (M1.4)
static void process_joystick(uint16_t raw, const char *neg_dir, const char *pos_dir,
                              const char **dir_out, uint8_t *pct_out)
{
    int32_t disp = (int32_t)raw - ADC_CENTER;
    uint32_t abs_disp = (uint32_t)(disp < 0 ? -disp : disp);

    if (abs_disp <= ADC_DEADZONE)
    {
        *dir_out = "Rest";
        *pct_out = 0;
    }
    else
    {
        uint32_t pct = abs_disp * 100 / ADC_CENTER;
        *pct_out = (uint8_t)(pct > 100 ? 100 : pct);
        *dir_out = disp < 0 ? neg_dir : pos_dir;
    }
}

void display_task_init(void)
{
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();

    // Initialize next run
    nextRun = HAL_GetTick() + DISPLAY_TASK_PERIOD_MS;
}

void display_task_execute(void)
{
    uint32_t now = HAL_GetTick();
    if (now >= nextRun)
    {
        uint16_t raw_x = adc_get_value(0);
        uint16_t raw_y = adc_get_value(1);

        // Determine direction and percentage for each axis (M1.4)
        const char *x_dir;
        uint8_t x_pct;
        const char *y_dir;
        uint8_t y_pct;

        process_joystick(raw_x, "Left", "Right", &x_dir, &x_pct);
        // Y-axis: negative displacement maps to "Up"; swap "Up"/"Down" if hardware is inverted
        process_joystick(raw_y, "Up",   "Down",  &y_dir, &y_pct);

        // Format display lines: "X:Right  75%"
        snprintf(buf_x, sizeof(buf_x), "X:%-5s %3d%%", x_dir, x_pct);
        snprintf(buf_y, sizeof(buf_y), "Y:%-5s %3d%%", y_dir, y_pct);

        // Update OLED display
        ssd1306_Fill(Black);
        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString(buf_x, Font_7x10, White);
        ssd1306_SetCursor(0, 12);
        ssd1306_WriteString(buf_y, Font_7x10, White);
        ssd1306_UpdateScreen();

        // Send raw ADC values over UART if enabled (M1.2, M1.3)
        if (uart_enabled)
        {
            snprintf(uart_buf, sizeof(uart_buf), "X:%4d Y:%4d\r\n", raw_x, raw_y);
            HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
        }

        // Schedule next run
        nextRun += DISPLAY_TASK_PERIOD_MS;
    }
}

