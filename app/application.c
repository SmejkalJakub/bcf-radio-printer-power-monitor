#include <application.h>

#define BATTERY_UPDATE_INTERVAL   (5 * 60 * 1000)
#define SEND_DATA_INTERVAL        (5 * 60 * 1000)
#define MEASURE_INTERVAL              (10 * 1000)


// LED instance
bc_led_t led;

// Button instance
bc_button_t button;

BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer_0, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer_1, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer_2, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer_3, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))

bc_data_stream_t sm_temperature_0;
bc_data_stream_t sm_temperature_1;
bc_data_stream_t sm_temperature_2;
bc_data_stream_t sm_temperature_3;

bc_data_stream_t *sm_temperature[] =
{
    &sm_temperature_0,
    &sm_temperature_1,
    &sm_temperature_2,
    &sm_temperature_3,
};

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    if (event == BC_BUTTON_EVENT_PRESS)
    {
        static uint16_t button_click_count = 0;

        // Pulse LED for 100 milliseconds
        bc_led_pulse(&led, 100);

        // Increment press count
        button_click_count++;

        bc_log_info("APP: Publish button press count = %u", button_click_count);

        // Publish button message on radio
        bc_radio_pub_push_button(&button_click_count);
    }
}

void battery_event_handler(bc_module_battery_event_t event, void *event_param)
{
    (void) event_param;

    float voltage;

    if (event == BC_MODULE_BATTERY_EVENT_UPDATE)
    {
        if (bc_module_battery_get_voltage(&voltage))
        {
            bc_radio_pub_battery(&voltage);
        }
    }
}

int adc_cntd;
float max;

void adc_measure_task(void *param)
{
    bc_adc_calibration();

    adc_cntd = 50;
    max = 0;

    bc_adc_async_measure(BC_ADC_CHANNEL_A4);

    bc_scheduler_plan_current_from_now(MEASURE_INTERVAL);
}

void adc_event_handler(bc_adc_channel_t channel, bc_adc_event_t event, void *event_param)
{
    if (event == BC_ADC_EVENT_DONE)
    {
        float result;
        float vdda_voltage;

        bc_adc_get_vdda_voltage(&vdda_voltage);

        bc_adc_async_get_voltage(channel, &result);

        float v = (vdda_voltage / 2) - (result);

        if (v > max)
        {
            max = v;
        }


        if (adc_cntd-- == 0)
        {
            float a = max * 15.f * 0.707;
            float w = a * 220 - 10.0;

            if (w < 0) w = 0;

            bc_log_debug("W %.2f", w);

            bc_radio_pub_float("watt", &w);
        }
        else
        {
            bc_adc_async_measure(BC_ADC_CHANNEL_A4);
        }

    }
}

void application_init(void)
{
    // Initialize logging
    bc_log_init(BC_LOG_LEVEL_DUMP, BC_LOG_TIMESTAMP_ABS);

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);

    // Initialize button
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize battery
    bc_module_battery_init();
    bc_module_battery_set_event_handler(battery_event_handler, NULL);
    bc_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    bc_adc_init();
    bc_adc_oversampling_set(BC_ADC_CHANNEL_A4, BC_ADC_OVERSAMPLING_256);
    bc_adc_set_event_handler(BC_ADC_CHANNEL_A4, adc_event_handler, NULL);
    bc_scheduler_register(adc_measure_task, NULL, 1000);

    bc_radio_init(BC_RADIO_MODE_NODE_SLEEPING);
    bc_radio_pairing_request("stetoskop", VERSION);

    bc_led_pulse(&led, 2000);
}