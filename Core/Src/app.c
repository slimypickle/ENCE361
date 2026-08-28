/*
 * app.c
 *
 * Application entry point and top-level cooperative scheduler.
 *
 * Task list + periods:
 *   button_task    - 20 ms    SW polling & debounce
 *   joystick_task  - 50 ms    ADC read & direction
 *   imu_task       - event    Interrupt-driven
 *   buzzer_task    - <1 ms    Non-blocking tone generation
 *   ui_task        - 50 ms    Display state machine (OLED redraws @ 250 ms)
 *   led_progress   - 250 ms   Goal-progress LED bar (DS1-DS4 + DS3 PWM)
 *   display_task   - 250 ms   UART serial debug
 *   blinky_task    - 500 ms   LD1 heartbeat
 *
 * ADC DMA buffer (3 x uint16_t, fixed-sequence ascending channel):
 *   raw_adc[0] = IN1  (PA1) - VR1 potentiometer
 *   raw_adc[1] = IN11 (PC4) - Joystick Y
 *   raw_adc[2] = IN12 (PC5) - Joystick X
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
#include "imu_task.h"
#include "led_progress.h"
#include "rgb.h"
#include "imu_lsm6ds.h"
#include "stm32c0xx_hal.h"


/* ADC DMA destination buffer                                           */

static uint16_t raw_adc[3];


/* ADC access API                                                       */

uint16_t adc_get_value(uint8_t index)
{
    if (index < 3) return raw_adc[index];
    return 0;
}

uint16_t adc_get_vr1(void)
{
    return raw_adc[0];
}


/* ADC DMA complete callback (no work needed — DMA writes raw_adc      */
/* in place, and consumers always read the latest value)               */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
}


/* Application entry point                                              */

void app_main(void)
{
    /* Data model first so all tasks see a sane initial state */
    sc_init();

    button_task_init();
    blinky_task_init();
    joystick_task_init();
    buzzer_init();

    /* IMU before the OLED display task. The IMU is on SPI2, the OLED
     * is on I2C1, so there is no bus conflict between them. The order
     * still matters for one reason: imu_init() blocks for ~100 ms
     * letting the LSM6DS3TR-C come out of boot. Doing it before
     * ssd1306_Init() kicks off its async DMA stream keeps the boot
     * sequence simple and deterministic. */
    imu_task_init();

    display_task_init();    /* UART debug */
    ui_task_init();         /* OLED state machine */

    /* LED progress bar — runs after ui_task so the OLED is already
     * up. Internally starts TIM2 CH3 for DS3 PWM. */
    led_progress_init();

    /* RGB power-on indicator (PD2/PD3/PD4 = green/red/blue).
     * All three channels on = white. The RGB LED is on its own
     * pins — independent of the DS1–DS4 progress bar — so this
     * does not conflict with led_progress. */
    rgb_colour_all_on();

// Main schedule loop
    while (1)
    {
        /* Start a new ADC DMA scan (VR1, Joy-Y, Joy-X).
         * Non-blocking — results land in raw_adc[]. */
        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)raw_adc, 3);

        /* Spec UI-4c: while Set Goal is active all other inputs are
         * ignored, so we skip the button task. */
        if (ui_get_state() != UI_SET_GOAL)
        {
            button_task_execute();
        }

        joystick_task_execute();
        imu_task_execute();      /* drains a pending IMU step interrupt */
        buzzer_task_execute();   /* needs every-loop polling for tone   */
        ui_task_execute();
        led_progress_update();   /* internally rate-limited to 250 ms   */
        display_task_execute();
        blinky_task_execute();
    }
}
