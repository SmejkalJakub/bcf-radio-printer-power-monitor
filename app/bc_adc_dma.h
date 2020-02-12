#ifndef _BC_ADC_DMA
#define _BC_ADC_DMA

#include <bc_common.h>

#include "bc_tick.h"
#include "bc_gpio.h"
#include "bc_adc.h"
#include "bc_scheduler.h"
#include "bc_dma.h"

void bc_adc_dma_init(uint8_t *buffer, size_t buffer_size);
bool bc_adc_dma_start(bc_adc_channel_t adc_channel, uint16_t prescaler, uint16_t autoreload);
bool bc_adc_dma_start_circular(bc_adc_channel_t adc_channel, uint16_t prescaler, uint16_t autoreload);
void bc_adc_dma_set_event_handler(void (*event_handler)(bc_dma_channel_t, bc_dma_event_t, void *), void *event_param);
bool bc_adc_dma_stop();

#endif
