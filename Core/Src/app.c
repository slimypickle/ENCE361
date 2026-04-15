/*
 * app.c
 *
 * Application entry point and top-level task scheduler.
 *
 * Task list and approximate periods:
 *   button_task   – 20 ms   SW polling & debounce
 *   joystick_task – 50 ms   ADC read & direction calculation
 *   buzzer_task   – called every loop (<1 ms) for tone generation
 *   ui_task       – 50 ms   display state machine (OLED redraws at 250 ms)
 *   display_task  – 250 ms  UART serial debug output
 *   blinky_task   – 500 ms  LD1 heartbeat
 *
 * ADC DMA buffer (3 × uint16_t, fixed sequence ascending channel):
 *   raw_adc[0] = IN1  (PA1) – VR1 potentiometer
 *   raw_adc[1] = IN11 (PC4) – Joystick Y
 *   raw_adc[2] = IN12 (PC5) – Joystick X
 */

#include "app.h"
#include "adc.h"
#include "gpio.h"
#include "tim.h"
#include "step_counter.h"
#include "button_task.h"
#include "blinky_task.h"
#include "joystick_task.h"
#include "buzzer.h"
#include "ui_task.h"
#include "display_task.h"
#include "stm32c0xx_hal.h"
#include "stm32c0xx_hal_tim.h"

/* ------------------------------------------------------------------ */
/* ADC DMA buffer  (3 channels)                                         */
/* ------------------------------------------------------------------ */
static uint16_t raw_adc[3];

/* ------------------------------------------------------------------ */
/* ADC access API                                                       */
/* ------------------------------------------------------------------ */
uint16_t adc_get_value(uint8_t index)
{
    if (index < 3) return raw_adc[index];
    return 0;
}

uint16_t adc_get_vr1(void)
{
    return raw_adc[0];
}

/* ------------------------------------------------------------------ */
/* ADC DMA complete callback (optional – kept for future use)           */
/* ------------------------------------------------------------------ */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
}

/* ------------------------------------------------------------------ */
/* Application entry point                                              */
/* ------------------------------------------------------------------ */
void app_main(void)
{
    /* Initialise data model first so all tasks see correct state */
    sc_init();

    /* Peripheral tasks */
    button_task_init();
    blinky_task_init();
    joystick_task_init();
    buzzer_init();
    display_task_init();  /* UART serial debug */
    ui_task_init();       /* OLED display state machine */

    /* ---- Main scheduler loop -------------------------------------- */
    while (1)
    {
        /* Start a new ADC DMA conversion (3 channels) */
        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)raw_adc, 3);

        /* Poll all tasks – each enforces its own period internally */
        /* Buttons are suppressed while Set Goal is active (M2.7b) */
        if (ui_get_state() != UI_SET_GOAL)
        {
            button_task_execute();
        }
        joystick_task_execute();
        buzzer_task_execute();
        ui_task_execute();
        display_task_execute();
        blinky_task_execute();
    }
}
