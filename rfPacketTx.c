#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <ti/drivers/rf/RF.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/ADC.h>
#include <ti/display/Display.h>
#include DeviceFamily_constructPath(driverlib/rf_prop_mailbox.h)
#include "ti_drivers_config.h"
#include <ti_radio_config.h>
/* Defines */
#define PAYLOAD_LENGTH 3+2+1 // Payload length for temperature, pulse data, and pulse rate
#define PACKET_INTERVAL 1000000 // 1s second packet interval
#define TEMPERATURE_THRESHOLD 2.0f // Threshold for temperature increase
#define ADC_RESOLUTION 4096 // 12-bit ADC resolution for CC1312R
#define FILTER_SIZE 10 // Adjust this value as needed
#define PEAK_THRESHOLD 1520 // Adjust this value based on your sensor's output range
#define MIN_PEAK_SPACING 200000000 // Minimum time between peaks in nanoseconds (equivalent to 300 BPM)
#define MAX_PEAK_SPACING 1000000000 // Maximum time between peaks in nanoseconds (equivalent to 40 BPM)
#define PULSE_BUFFER_SIZE 10 // Buffer size for storing pulse periods


float baseline_temperature;
struct timespec last_pulse_time = {0};
uint32_t pulse_period = 0;
float pulse_rate = 0.0;

/* Variable declarations */
static RF_Object rfObject;
static RF_Handle rfHandle;
static ADC_Handle adc_temperature, adc_pulse;
static uint8_t packet[PAYLOAD_LENGTH];

static uint16_t filter_buffer[FILTER_SIZE] = {0};
static uint8_t filter_index = 0;

static uint64_t pulse_periods[PULSE_BUFFER_SIZE] = {0};
static uint8_t pulse_buffer_index = 0;
static uint8_t pulse_buffer_count = 0;


#define MIN_PEAK_SPACING 200000000 // Minimum time between peaks in nanoseconds (equivalent to 300 BPM)
#define MAX_PEAK_SPACING 1000000000 // Maximum time between peaks in nanoseconds (equivalent to 40 BPM)

float calculate_pulse_rate(uint16_t adc_value) {
    static struct timespec last_peak_time = {0};
    static uint64_t peak_count = 0;
    static uint64_t total_peak_spacing = 0;
    struct timespec current_time;

    static uint16_t prev_filtered_value = 0;
    uint32_t sum = 0;
    uint16_t filtered_value;

    // Insert new ADC value into the filter buffer
    sum -= filter_buffer[filter_index];
    filter_buffer[filter_index] = adc_value;
    sum += adc_value;
    filter_index = (filter_index + 1) % FILTER_SIZE;

    // Calculate the moving average
    filtered_value = sum / FILTER_SIZE;

    // Calculate dynamic peak spacing thresholds
    uint64_t min_peak_spacing, max_peak_spacing;
    if (peak_count > 0) {
        uint64_t average_peak_spacing = total_peak_spacing / peak_count;
        float current_pulse_rate = 60.0 * 1000000000 / average_peak_spacing;
        min_peak_spacing = 1000000000 / (current_pulse_rate * 1.2); // 20% higher than current rate
        max_peak_spacing = 1000000000 / (current_pulse_rate * 0.8); // 20% lower than current rate
    } else {
        min_peak_spacing = MIN_PEAK_SPACING;
        max_peak_spacing = MAX_PEAK_SPACING;
    }

    // Detect pulse peak using slope
    if (filtered_value > prev_filtered_value) {
        // Rising slope
    } else if (filtered_value < prev_filtered_value) {
        // Falling slope
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        uint64_t peak_spacing = (current_time.tv_sec - last_peak_time.tv_sec) * 1000000000 +
                                (current_time.tv_nsec - last_peak_time.tv_nsec);

        // Validate peak spacing
        if (peak_spacing >= min_peak_spacing && peak_spacing <= max_peak_spacing) {
            peak_count++;
            total_peak_spacing += peak_spacing;
            last_peak_time = current_time;
        }
    }

    prev_filtered_value = filtered_value;

    // Calculate pulse rate based on the average peak spacing
    if (peak_count > 0) {
        uint64_t average_peak_spacing = total_peak_spacing / peak_count;
        float uncalibrated_pulse_rate = 60.0 * 1000000000 / average_peak_spacing;

                // Calibrate the pulse rate by subtracting 220
                float calibrated_pulse_rate = uncalibrated_pulse_rate - 210.0;

                return calibrated_pulse_rate;
    }

    return 0.0;
}
void *mainThread(void *arg0) {
    RF_Params rfParams;
    ADC_Params adcParams_temperature, adcParams_pulse;
    uint16_t adcValue_temperature, adcValue_pulse;
    float temperature;

    GPIO_setConfig(CONFIG_GPIO_GLED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_write(CONFIG_GPIO_GLED, CONFIG_GPIO_LED_OFF);

    RF_cmdPropTx.pktLen = PAYLOAD_LENGTH;
    RF_cmdPropTx.pPkt = packet;
    RF_cmdPropTx.startTrigger.triggerType = TRIG_NOW;

    // RF Driver Initialization
    RF_Params_init(&rfParams);
    rfHandle = RF_open(&rfObject, &RF_prop, (RF_RadioSetup*)&RF_cmdPropRadioDivSetup, &rfParams);
    if (rfHandle == NULL) {
        printf("Error: Failed to initialize RF driver\n");
        while (1) {}
    }


    RF_postCmd(rfHandle, (RF_Op*)&RF_cmdFs, RF_PriorityNormal, NULL, 0);

    // Delay for RF driver initialization
    usleep(1000000); // 1000ms delay

    /* Initialize ADC for temperature sensor */
    ADC_init();
    ADC_Params_init(&adcParams_temperature);
    adc_temperature = ADC_open(CONFIG_ADC_0, &adcParams_temperature);
    if (adc_temperature == NULL) {
        printf("Error: Failed to initialize ADC driver for temperature sensor\n");
        while (1) {}
    }

    /* Initialize ADC for pulse sensor */
    ADC_Params_init(&adcParams_pulse);
    adc_pulse = ADC_open(CONFIG_ADC_1, &adcParams_pulse);
    if (adc_pulse == NULL) {
        printf("Error: Failed to initialize ADC driver for pulse sensor\n");
        while (1) {}
    }

    while (1) {
        /* Read temperature data from LM35 sensor */
        ADC_convert(adc_temperature, &adcValue_temperature);

        // Print raw ADC value
//        printf("Raw ADC value (temperature): %d\n", adcValue_temperature);

        // Print voltage value
        float voltage_temperature = ((float)adcValue_temperature * 3.3) / (ADC_RESOLUTION-1);
//        printf("Voltage value (temperature): %.2f V\n", voltage_temperature);

        temperature = voltage_temperature * 100; // Scale factor of 10 mV/°C for LM35

        if (temperature > baseline_temperature + TEMPERATURE_THRESHOLD) {
            printf("Temperature increased to: %.2f C\n", temperature);//140 144 149
            // (Optional) Reset baseline temperature after a short timeout
        } else {
            // Update baseline temperature if no significant change is detected
            baseline_temperature = temperature;
        }

        /* Read pulse data from analog pulse sensor */
        ADC_convert(adc_pulse, &adcValue_pulse);

        // Print raw ADC value
        printf("Raw ADC value (pulse): %d\n", adcValue_pulse);

        /* Calculate pulse rate */
        pulse_rate = calculate_pulse_rate(adcValue_pulse);

        /* Prepare packet with temperature, pulse data, and pulse rate */
                packet[0] = (uint8_t)(temperature); // Integer part of temperature
                packet[1] = (uint8_t)((temperature - (int)temperature) * 100); // Fractional part of temperature
                packet[2] = (uint8_t)(adcValue_pulse >> 8); // High byte of pulse ADC value
                packet[3] = (uint8_t)(adcValue_pulse & 0xFF); // Low byte of pulse ADC value
                packet[4] = (uint8_t)(pulse_rate); // Integer part of pulse rate
                packet[5] = (uint8_t)((pulse_rate - (int)pulse_rate) * 100); // Fractional part of pulse rate
//
//                printf("Transmitting packet: [ %.2f]\n",  pulse_rate);
              printf("Transmitting packet: [%d.%02d, %d]\n", packet[0], packet[1],packet[4]);

                /* Send packet */
                RF_EventMask terminationReason = RF_runCmd(rfHandle, (RF_Op*)&RF_cmdPropTx, RF_PriorityNormal, NULL, 0);

              // ... (existing error handling code) ...
              if (terminationReason & RF_EventCmdPreempted) {
                    printf("Error: RF command preempted\n");
                } else if (terminationReason & RF_EventCmdAborted) {
                    printf("Error: RF command aborted\n");
                } else {
                    // Assume transmission successful if no other error conditions are met
                    GPIO_toggle(CONFIG_GPIO_GLED);
                }

                /* Power down the radio */
                RF_yield(rfHandle);

                /* Sleep for PACKET_INTERVAL us */
                usleep(PACKET_INTERVAL);
            }
        }
