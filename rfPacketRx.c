#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <ti/drivers/rf/RF.h>
#include DeviceFamily_constructPath(driverlib/rf_prop_mailbox.h)
#include "ti_drivers_config.h"
#include <ti_radio_config.h>

/* Defines */
#define PAYLOAD_LENGTH 6 // Payload length for temperature, pulse data, and pulse rate

/* Variable declarations */
static RF_Object rfObject;
static RF_Handle rfHandle;

void *mainThread(void *arg0) {
    RF_Params rfParams;
    uint8_t rxPacket[PAYLOAD_LENGTH];
    float receivedTemperature, receivedPulseRate;
    uint16_t receivedPulseData;

    // RF Driver Initialization for Receiver
    RF_Params_init(&rfParams);
    rfHandle = RF_open(&rfObject, &RF_prop, (RF_RadioSetup*)&RF_cmdPropRadioDivSetup, &rfParams);
    if (rfHandle == NULL) {
        printf("Error: Failed to initialize RF driver for receiver\n");
        while (1) {}
    }

    // Set the frequency
    RF_postCmd(rfHandle, (RF_Op*)&RF_cmdFs, RF_PriorityNormal, NULL, 0);

    // Delay for RF driver initialization
    usleep(1000000); // 1000ms delay

    while (1) {
        // Configure receive command
        RF_cmdPropRx.maxPktLen = PAYLOAD_LENGTH;

        // Receive packet
        RF_EventMask terminationReason = RF_runCmd(rfHandle, (RF_Op*)&RF_cmdPropRx, RF_PriorityNormal, NULL, 0);

        if (terminationReason & RF_EventRxOk) {
            // Parse received packet
            receivedTemperature = (float)rxPacket[0] + ((float)rxPacket[1] / 100.0);
            receivedPulseData = (rxPacket[2] << 8) | rxPacket[3];
            receivedPulseRate = (float)rxPacket[4] + ((float)rxPacket[5] / 100.0);

            // Print received data
            printf("Received Temperature: %.2f C\n", receivedTemperature);
            printf("Received Pulse Data: %d\n", receivedPulseData);
            printf("Received Pulse Rate: %.2f BPM\n", receivedPulseRate);
        } else {
            printf("Error: Failed to receive packet\n");
        }

        // Power down the radio
        RF_yield(rfHandle);
    }
}
