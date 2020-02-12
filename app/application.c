#include <application.h>

#include "bc_adc_dma.h"
#include "bc_adc.h"

// LED instance
bc_led_t led;

// Button instance
bc_button_t button;

/*
If no PLL is NOT enabled, then the core clock is 2 MHz

Prescaler:
2 MHz / 1000 = 2 kHz = 2000 Hz
Autoreload:
2000 Hz / 2000 = 1 Hz
*/
#define ADC_DMA_1HZ_PRESCALER 1000
#define ADC_DMA_1HZ_AUTORELOAD 2000

#define ADC_DMA_2HZ_PRESCALER 1000
#define ADC_DMA_2HZ_AUTORELOAD 1000

#define ADC_DMA_10HZ_PRESCALER 1000
#define ADC_DMA_10HZ_AUTORELOAD 200

#define ADC_DMA_100HZ_PRESCALER 1000
#define ADC_DMA_100HZ_AUTORELOAD 20

uint8_t buffer[16];
uint8_t dma_flag = 0;

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    if (event == BC_BUTTON_EVENT_PRESS)
    {
        bc_led_set_mode(&led, BC_LED_MODE_TOGGLE);
        //bc_adc_dma_start_circular(BC_ADC_CHANNEL_A0, ADC_DMA_1HZ_PRESCALER, ADC_DMA_1HZ_AUTORELOAD);
    }
}

static void adc_dma_event_handler(bc_dma_channel_t channel, bc_dma_event_t event, void *event_param)
{
    (void) event_param;
    (void) channel;

    if (event == BC_DMA_EVENT_HALF_DONE)
    {
        //bc_log_debug("First half of buffer is full");
        dma_flag = 1;
    }
    else if (event == BC_DMA_EVENT_DONE)
    {
        //bc_log_debug("Second half of buffer is full");
        dma_flag = 2;
        // If you are not using CIRCULAR, you have to stop DMA manually by uncommening line below
        // bc_adc_dma_stop();
    }
}

void application_init(void)
{
    // Initialize logging
    bc_log_init(BC_LOG_LEVEL_DUMP, BC_LOG_TIMESTAMP_ABS);

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_ON);

    // Initialize button
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    bc_adc_dma_init(buffer, sizeof(buffer));
    bc_adc_dma_set_event_handler(adc_dma_event_handler, NULL);

    // Start circular ADC sampling on P0/A0 to the buffer
    bc_adc_dma_start_circular(BC_ADC_CHANNEL_A0, ADC_DMA_2HZ_PRESCALER, ADC_DMA_2HZ_AUTORELOAD);

    // Non-circular ADC sampling on P0/A0 to the buffer
    //bc_adc_dma_start(BC_ADC_CHANNEL_A0, ADC_DMA_1HZ_PRESCALER, ADC_DMA_1HZ_AUTORELOAD);

    bc_scheduler_disable_sleep();
}

void application_task()
{
    // This task is just to print the filling buffer and displaying which
    // half of the buffer could be used for transfer/computation
    bc_log_dump(buffer, sizeof(buffer), "Buffer: %d", dma_flag);
    bc_scheduler_plan_current_relative(300);
}