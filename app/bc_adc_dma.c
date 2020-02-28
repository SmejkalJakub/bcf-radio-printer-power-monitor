#include "bc_adc_dma.h"


typedef struct
{
    uint16_t *buffer;
    size_t buffer_size;
    bc_adc_channel_t adc_channel;
    bool circular;

    uint16_t prescaler;
    uint16_t autoreload;

    bool running;

} bc_adc_dma;

static bc_adc_dma _bc_adc_dma;

static void _bc_adc_dma_adc_init(void);
static void _bc_adc_dma_tim6_init(uint16_t prescaler, uint16_t autoreload);
static void _bc_adc_dma_dma_init(void);

void bc_adc_dma_init(uint16_t *buffer, size_t buffer_size)
{
    memset(&_bc_adc_dma, 0, sizeof(_bc_adc_dma));

    _bc_adc_dma.buffer = buffer;
    _bc_adc_dma.buffer_size = buffer_size;

    bc_dma_init();

    _bc_adc_dma_adc_init();
    _bc_adc_dma_dma_init();
}

void _bc_adc_dma_start()
{
    bc_scheduler_disable_sleep();

    _bc_adc_dma_tim6_init(_bc_adc_dma.prescaler, _bc_adc_dma.autoreload);

    // Reinit the DMA
    _bc_adc_dma_dma_init();

    // Set ADC channel
    ADC1->CHSELR = 1 << _bc_adc_dma.adc_channel;

    // Enable DMA - this must be set after calibration phase!
    ADC1->CFGR1 |= ADC_CFGR1_DMAEN;

    if (_bc_adc_dma.circular)
    {
        ADC1->CFGR1 |= ADC_CFGR1_DMACFG;
    }

    // Configure trigger by timer TRGO signal
    // Rising edge, TRG0 (TIM6_TRGO)
    ADC1->CFGR1 |= ADC_CFGR1_EXTEN_0;

    // Start the AD measurement
    ADC1->CR |= ADC_CR_ADSTART;

    // Start sampling timer
    TIM6->CR1 |= TIM_CR1_CEN;

    _bc_adc_dma.running = true;
}

bool bc_adc_dma_start(bc_adc_channel_t adc_channel, uint16_t prescaler, uint16_t autoreload)
{
    if (_bc_adc_dma.running)
    {
        return false;
    }

    _bc_adc_dma.prescaler = prescaler;
    _bc_adc_dma.autoreload = autoreload;
    _bc_adc_dma.adc_channel = adc_channel;
    _bc_adc_dma.circular = false;

    _bc_adc_dma_start();

    return true;
}

bool bc_adc_dma_start_circular(bc_adc_channel_t adc_channel, uint16_t prescaler, uint16_t autoreload)
{
    if (_bc_adc_dma.running)
    {
        return false;
    }

    _bc_adc_dma.prescaler = prescaler;
    _bc_adc_dma.autoreload = autoreload;
    _bc_adc_dma.adc_channel = adc_channel;
    _bc_adc_dma.circular = true;

    _bc_adc_dma_start();

    return true;
}

bool bc_adc_dma_stop()
{
    // Disable counter
    TIM6->CR1 &= ~TIM_CR1_CEN;

    bc_dma_channel_stop(BC_DMA_CHANNEL_1);

    _bc_adc_dma.running = false;

    return true;
}

static void _bc_adc_dma_tim6_init(uint16_t prescaler, uint16_t autoreload)
{
    // Enable TIM6 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    // Errata workaround
    RCC->APB1ENR;

    // Disable counter if it is running
    TIM6->CR1 &= ~TIM_CR1_CEN;

    // Set prescaler
    //TIM6->PSC = 10 - 1;
    TIM6->PSC = prescaler - 1;

    // Set auto-reload
    //TIM6->ARR = 100 - 1;
    TIM6->ARR = autoreload - 1;

    // Configure update event TRGO to ADC
    TIM6->CR2 |= TIM_CR2_MMS_1;
}

static void _bc_adc_dma_adc_init(void)
{
    // Enable ADC clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    // Errata workaround
    RCC->APB2ENR;

    // Set auto-off mode, right align
    ADC1->CFGR1 = ADC_CFGR1_AUTOFF;

    // Set 8bit resolution
    ADC1->CFGR1 |= ADC_CFGR1_RES_1;

    // Set PCLK as a clock source
    ADC1->CFGR2 = ADC_CFGR2_CKMODE_0 | ADC_CFGR2_CKMODE_1;

    // Sampling time selection (12.5 cycles)
    ADC1->SMPR |= ADC_SMPR_SMP_1 | ADC_SMPR_SMP_0;

    // Enable ADC voltage regulator
    ADC1->CR |= ADC_CR_ADVREGEN;
}

static void _bc_adc_dma_dma_init(void)
{
    static bc_dma_channel_config_t _bc_spi_dma_config =
    {
        .request = BC_DMA_REQUEST_0,
        .direction = BC_DMA_DIRECTION_TO_RAM,
        .data_size_memory = BC_DMA_SIZE_2,
        .data_size_peripheral = BC_DMA_SIZE_2,
        .mode = BC_DMA_MODE_CIRCULAR,
        .address_peripheral = (void *)&ADC1->DR,
        .priority = BC_DMA_PRIORITY_HIGH
    };

    // Setup DMA channel
    _bc_spi_dma_config.address_memory = (void *)_bc_adc_dma.buffer;
    _bc_spi_dma_config.length = _bc_adc_dma.buffer_size;

    _bc_spi_dma_config.mode = (_bc_adc_dma.circular) ? BC_DMA_MODE_CIRCULAR : BC_DMA_MODE_STANDARD;

    bc_dma_channel_config(BC_DMA_CHANNEL_1, &_bc_spi_dma_config);
    bc_dma_channel_run(BC_DMA_CHANNEL_1);
}

void bc_adc_dma_set_event_handler(void (*event_handler)(bc_dma_channel_t, bc_dma_event_t, void *), void *event_param)
{
    bc_dma_set_event_handler(BC_DMA_CHANNEL_1, event_handler, event_param);
}