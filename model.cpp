/** 
 *  @file           model.cpp
 *  @author         Arella Matteo <br/>
 *                  (mail: arella.1646983@studenti.uniroma1.it)
 *  @date           2018
 *  @brief          Board model implementation file
 */
#include "model.h"
#include "filter.h"
#include "can_servizi.h"

#if 0
#include <DueFlashStorage.h>
#endif

#undef HID_ENABLED

/** @addtogroup Board_model_group
 *   @{
 */

/**
 *  @def ADC_BUFFER_SIZE
 *  @brief Size (bytes) of buffer for store each ADC channel data with DMA
 */
#define ADC_BUFFER_SIZE         128

/**
 *  @def        BUFFERS
 *  @brief      Number of DMA buffers
 *  @warning    Must be a power of two
 */
#define BUFFERS                 4

/**
 *  @def ADC_MIN
 *  @brief ADC lower bound value
 */
#define ADC_MIN                 0

/**
 *  @def ADC_MAX
 *  @brief ADC upper bound value
 */
#define ADC_MAX                 4095

/**
 *  @def ADC_CHANNELS_LIST
 *  @brief List of ADC channels dedicated to each IO port
 */
#define ADC_CHANNELS_LIST       TPS1_ADC_CHAN_NUM | TPS2_ADC_CHAN_NUM | BRAKE_ADC_CHAN_NUM | SC_ADC_CHAN_NUM

/**
 *  @def ADC_CHANNELS
 *  @brief Number of ADC channels
 */
#define ADC_CHANNELS            4

/**
 *  @def TPS1_ADC_OFFSET
 *  @brief Offset from DMA buffer
 */
#define TPS1_ADC_OFFSET         3

/**
 *  @def TPS2_ADC_OFFSET
 *  @brief Offset from DMA buffer
 */
#define TPS2_ADC_OFFSET         2

/**
 *  @def BRAKE_ADC_OFFSET
 *  @brief Offset from DMA buffer
 */
#define BRAKE_ADC_OFFSET        1

/**
 *  @def SC_ADC_OFFSET
 *  @brief Offset from DMA buffer
 */
#define SC_ADC_OFFSET           0

/**
 *  @def BUFFER_LENGTH
 *  @brief Length, in bytes, of each DMA buffer
 */
#define BUFFER_LENGTH           ADC_BUFFER_SIZE * ADC_CHANNELS

/**
 *  @def APPS_PLAUS_RANGE
 *  @brief Maximum percentage deviation of pedal travel between two APPS
 */
#define APPS_PLAUS_RANGE        10

/**
 * @var     volatile uint8_t tps1_adc_percentage;
 * @brief   First APPS percentage value retrieved by analog tps1 signal (#TPS1_PIN)
 */
volatile uint8_t    tps1_adc_percentage    = 0;

/**
 * @var     volatile uint8_t tps2_adc_percentage;
 * @brief   Second APPS percentage value retrieved by analog tps2 signal (#TPS2_PIN)
 */
volatile uint8_t    tps2_adc_percentage    = 0;

/**
 * @var     volatile uint8_t brake_adc_percentage;
 * @brief   Brake pedal position sensor percentage value retrieved by analog brake
 *          signal (#BRAKE_PIN)
 */
volatile uint8_t    brake_adc_percentage   = 0;

/**
 * @var     volatile bool apps_adc_plausibility;
 * @brief   APPS plausibility status retrieved by analog acquisition
 */
volatile bool       apps_adc_plausibility  = true;

/**
 * @var     volatile bool brake_adc_plausibility;
 * @brief   Brake plausibility status retrieved by analog acquisition
 */
volatile bool       brake_adc_plausibility = true;

/**
 * @var     volatile uint16_t tps1_value;
 * @brief   First APPS value retrieved directly by analog tps1 signal (#TPS1_PIN)
 *          and filtered after DMA buffer is filled entirely
 */
volatile uint16_t   tps1_value = 0;

/**
 * @var     volatile uint16_t tps2_value;
 * @brief   Second APPS value retrieved directly by analog tps2 signal (#TPS2_PIN)
 *          and filtered after DMA buffer is filled entirely
 */
volatile uint16_t   tps2_value = 0;

/**
 * @var     volatile uint16_t brake_value;
 * @brief   Brake pedal position sensor value retrieved directly by analog brake
 *          signal (#BRAKE_PIN) and filtered after DMA buffer is filled entirely
 */
volatile uint16_t   brake_value = 0;

/**
 * @var     volatile uint16_t SC_value;
 * @brief   SC value retrieved directly by analog SC signal (#SC_PIN)
 *          and filtered after DMA buffer is filled entirely
 */
volatile uint16_t   SC_value    = 0;

/**
 * @def     TPS1_UPPER_BOUND
 * @brief   First APPS max output voltage (1.1V)
 */
#define TPS1_UPPER_BOUND            1365

/**
 * @def     TPS1_LOWER_BOUND
 * @brief   First APPS min output voltage (0.4V)
 */
#define TPS1_LOWER_BOUND            496

/**
 * @def     TPS2_UPPER_BOUND
 * @brief   Second APPS max output voltage (2.35V)
 */
#define TPS2_UPPER_BOUND            2916

/**
 * @def     TPS2_LOWER_BOUND
 * @brief   Second APPS min output voltage (0.8V)
 */
#define TPS2_LOWER_BOUND            992

/**
 * @def     BRAKE_UPPER_BOUND
 * @brief   Brake sensor max output voltage (2.85V)
 */
#define BRAKE_UPPER_BOUND           3536

/**
 * @def     BRAKE_LOWER_BOUND
 * @brief   Brake sensor min output voltage (2V)
 */
#define BRAKE_LOWER_BOUND           2500

/**
 *  @var    volatile uint16_t tps1_max;
 *  @brief  First APPS max output voltage (equals to #TPS1_UPPER_BOUND)
 */
volatile uint16_t   tps1_max    = TPS1_UPPER_BOUND;

/**
 *  @var    volatile uint16_t tps1_min;
 *  @brief  First APPS min output voltage (equals to #TPS1_LOWER_BOUND)
 */
volatile uint16_t   tps1_min    = TPS1_LOWER_BOUND;

/**
 *  @var    volatile uint16_t tps2_max;
 *  @brief  Second APPS max output voltage (equals to #TPS2_UPPER_BOUND)
 */
volatile uint16_t   tps2_max    = TPS2_UPPER_BOUND;

/**
 *  @var    volatile uint16_t tps2_min;
 *  @brief  Second APPS min output voltage (equals to #TPS2_LOWER_BOUND)
 */
volatile uint16_t   tps2_min    = TPS2_LOWER_BOUND;

/**
 *  @var    volatile uint16_t brake_max;
 *  @brief  Brake sensor max output voltage (equals to #BRAKE_UPPER_BOUND)
 */
volatile uint16_t   brake_max   = BRAKE_UPPER_BOUND;

/**
 *  @var    volatile uint16_t brake_min;
 *  @brief  Brake sensor min output voltage (equals to #BRAKE_LOWER_BOUND)
 */
volatile uint16_t   brake_min   = BRAKE_LOWER_BOUND;

/**
 *  @var    volatile int bufn;
 *  @brief  DMA buffer pointer
 */
volatile int        bufn;

/**
 *  @var    volatile int obufn;
 *  @brief  DMA buffer pointer
 */
volatile int        obufn;

/**
 * @var     volatile uint16_t   buf[#BUFFERS][#BUFFER_LENGTH];
 * @brief   DMA buffers: #BUFFERS number of buffers each of #BUFFER_LENGTH size;
 *          DMA is configured in cyclic mode: after one of #BUFFERS is filled then 
 *          DMA transfer head moves to next buffer in circular indexing.
 */
volatile uint16_t   buf[BUFFERS][BUFFER_LENGTH];

/**
 *  @brief      This function filters ADC acquisitions;
 *              - Each ADC buffer data is filtered and an average is done with 
 *                  previous values; 
 *              - Each pedal filtered value is mapped to a percentage value;
 *              - APPS plausibility and brake plausibility are checked.
 *              
 *              
 *  @author     Arella Matteo <br/>
 *              (mail: arella.1646983@studenti.uniroma1.it)
 */
static inline void filter_data() {
    tps1_value = (tps1_value + filter_buffer(buf[obufn] + TPS1_ADC_OFFSET, ADC_BUFFER_SIZE, ADC_CHANNELS)) / 2;
    tps2_value = (tps2_value + filter_buffer(buf[obufn] + TPS2_ADC_OFFSET, ADC_BUFFER_SIZE, ADC_CHANNELS)) / 2;
    brake_value = (brake_value + filter_buffer(buf[obufn] + BRAKE_ADC_OFFSET, ADC_BUFFER_SIZE, ADC_CHANNELS)) / 2;
    SC_value = (SC_value + filter_buffer(buf[obufn] + SC_ADC_OFFSET, ADC_BUFFER_SIZE, ADC_CHANNELS)) / 2;

    if (tps1_value < tps1_min) tps1_value = tps1_min;
    if (tps1_value > tps1_max) tps1_value = tps1_max;
    if (tps2_value < tps2_min) tps2_value = tps2_min;
    if (tps2_value > tps2_max) tps2_value = tps2_max;
    if (brake_value < brake_min) brake_value = brake_min;
    if (brake_value > brake_max) brake_value = brake_max;

    tps1_adc_percentage = map(tps1_value, tps1_min, tps1_max, 0, 100);
    tps2_adc_percentage = map(tps2_value, tps2_min, tps2_max, 0, 100);
    brake_adc_percentage = map(brake_value, brake_min, brake_max, 0, 100);

    // check APPS plausibility
    if (abs(tps1_adc_percentage - tps2_adc_percentage) > APPS_PLAUS_RANGE)
      apps_adc_plausibility = false;
    else
      apps_adc_plausibility = true;

    // check APPS + brake plausibility (BSPD)
    if (tps1_adc_percentage > 5 && brake_adc_percentage > 25) // ACCELERATOR + BRAKE plausibility
      brake_adc_plausibility = false;
    else if (!brake_adc_percentage)
      brake_adc_plausibility = true;
}

/**
 *  @brief      ADC IRQ handler.
 *              When ADC buffer is filled DMA pointer is linked to next buffer
 *              available.
 *              Then acquired data are filtered.
 *              
 *  @author     Arella Matteo <br/>
 *              (mail: arella.1646983@studenti.uniroma1.it)
 */
void ADC_Handler() {
    int f = ADC->ADC_ISR;
    if (f & (1 << 27)){
        bufn = (bufn + 1) & (BUFFERS - 1); // move DMA pointers to next buffer
        ADC->ADC_RNPR = (uint32_t) buf[bufn];
        ADC->ADC_RNCR = BUFFER_LENGTH;

        filter_data();
    
        obufn = (obufn + 1) &(BUFFERS - 1);
    }
}

/**
 *  @brief      This function initializes hardware board.
 *              Both APPS and brake pedal position sensor are acquired by analog
 *              signals for fault tolerance in case of CAN servizi fault.
 *
 *              ADC peripheral is initialized with ADC_FREQ_MAX clock and with
 *              12bits of resolution.
 *
 *              ADC peripheral is then configured in free running mode for continuous
 *              acquisitions.
 *
 *              ADC channels are enabled according to #ADC_CHANNELS_LIST.
 *
 *              ADC End of Receive Buffer Interrupt is enabled for trigger interrupt
 *              when DMA has filled entire buffer.
 *
 *              Then digital GPIO ports for AIR+, AIR-, PRE, BUZZER, AIRbutton,
 *              RTDbutton are enabled.
 *              
 *  @author     Arella Matteo <br/>
 *              (mail: arella.1646983@studenti.uniroma1.it)
 */
void model_init() {
    pmc_enable_periph_clk(ID_ADC);
    adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST);
    adc_set_resolution(ADC, ADC_12_BITS);

    ADC->ADC_MR |=0x80; // free running

    ADC->ADC_CHER = 0x00;
    ADC->ADC_CHER = ADC_CHANNELS_LIST;

    NVIC_EnableIRQ(ADC_IRQn);
    ADC->ADC_IDR = ~ADC_IDR_ENDRX; //~(1<<27); End of Receive Buffer Interrupt Disable
    ADC->ADC_IER = ADC_IDR_ENDRX; // 1<<27; End of Receive Buffer Interrupt Enable
    ADC->ADC_RPR = (uint32_t) buf[0];   // DMA buffer
    ADC->ADC_RCR = BUFFER_LENGTH;
    ADC->ADC_RNPR = (uint32_t) buf[1]; // next DMA buffer
    ADC->ADC_RNCR = BUFFER_LENGTH;
    bufn = obufn = 1;
    ADC->ADC_PTCR = 1;
    ADC->ADC_CR = 2;

    pinMode(FAN, OUTPUT);

    pinMode(RTDB, INPUT);

    Serial.begin(SERIAL_BAUDRATE);

    analogWriteResolution(12);

    // model_enable_calibrations();
}

__attribute__((__inline__)) volatile uint8_t get_tps1_percentage() {
    return can_servizi_online() ? get_servizi_tps1() : tps1_adc_percentage;
}

__attribute__((__inline__)) volatile uint8_t get_tps2_percentage() {
    return can_servizi_online() ? get_servizi_tps2() : tps2_adc_percentage;
}

__attribute__((__inline__)) volatile uint8_t get_brake_percentage() {
    return can_servizi_online() ? get_servizi_brake() : brake_adc_percentage;
}

__attribute__((__inline__)) volatile bool    get_apps_plausibility() {
    return can_servizi_online() ? get_servizi_apps_plausibility() : apps_adc_plausibility;
}

__attribute__((__inline__)) volatile bool    get_brake_plausibility() {
    return can_servizi_online() ? get_servizi_brake_plausibility() : brake_adc_plausibility;
}

__attribute__((__inline__)) volatile uint16_t get_SC_value() {
    return SC_value;
}

/**
 *  @}
 */
