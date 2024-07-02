 # VitalBeam: Long-Range Wireless Health Monitor

## Overview

VitalBeam is a cutting-edge wireless health monitoring system built on the Texas Instruments CC1312R microcontroller. This innovative solution combines real-time vital sign monitoring with long-range wireless transmission capabilities up to 500 meters. Designed for applications in remote patient care, large-scale health facilities, and IoT healthcare, VitalBeam offers precise biometric data collection and seamless long-distance data transmission.

## Features

- Real-time temperature monitoring using LM35 sensor
- Pulse rate detection and calculation
- Long-range wireless data transmission (up to 500 meters)
- Adaptive thresholding for accurate pulse detection
- LED status indicator
- Energy-efficient operation

## Hardware Requirements

- Texas Instruments CC1312R LaunchPad (2 units: transmitter and receiver)
- LM35 Temperature Sensor
- Analog Pulse Sensor
- LED for status indication

## Software Dependencies

VitalBeam relies on the following TI-RTOS drivers and libraries:

- TI-RTOS (Real-Time Operating System)
- TI RF Driver (`<ti/drivers/rf/RF.h>`)
- TI GPIO Driver (`<ti/drivers/GPIO.h>`)
- TI ADC Driver (`<ti/drivers/ADC.h>`)
- TI Display Driver (`<ti/display/Display.h>`)
- TI RF Mailbox Driver (`DeviceFamily_constructPath(driverlib/rf_prop_mailbox.h)`)
- TI Drivers Config (`"ti_drivers_config.h"`)
- TI Radio Config (`<ti_radio_config.h>`)

Standard C libraries used:
- `<time.h>`, `<stdlib.h>`, `<unistd.h>`, `<stdint.h>`, `<stdio.h>`, `<math.h>`

## Installation and Setup

1. Clone this repository to your local machine.
2. Open the project in Code Composer Studio (CCS) or your preferred TI development environment.
3. Ensure you have the TI-RTOS and CC1312R SDK installed.
4. Configure your project to use the correct board support package (BSP) for CC1312R.
5. Build and flash the transmitter code to one CC1312R LaunchPad.
6. Build and flash the receiver code to another CC1312R LaunchPad.

## Usage

### Transmitter:
1. Connect the LM35 temperature sensor to CONFIG_ADC_0.
2. Connect the analog pulse sensor to CONFIG_ADC_1.
3. Power on the transmitter CC1312R LaunchPad.
4. The system will start monitoring and transmitting vital signs at 1-second intervals.

### Receiver:
1. Power on the receiver CC1312R LaunchPad.
2. The receiver will continuously listen for incoming data packets.
3. Received vital signs will be printed to the console.

## Key Components

- `calculate_pulse_rate()`: Pulse detection algorithm using moving average and adaptive thresholding.
- `mainThread()`: Main operational loop for both transmitter and receiver.

## Customization

- `TEMPERATURE_THRESHOLD`: Adjust sensitivity of temperature increase detection.
- `FILTER_SIZE`: Modify the moving average filter for pulse detection.
- `MIN_PEAK_SPACING` and `MAX_PEAK_SPACING`: Fine-tune the acceptable range for pulse rate calculation.

## Contributing

Contributions to VitalBeam are welcome! Please feel free to submit pull requests, create issues or spread the word.

## License

[Insert your chosen license here]

## Acknowledgements

VitalBeam utilizes Texas Instruments' CC1312R microcontroller and associated development tools. Special thanks to the TI development community for their comprehensive documentation and support.
