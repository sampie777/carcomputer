//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_CONFIG_H
#define APP_TEMPLATE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "secrets.h"

#define DEVICE_NAME "Nissan Micra"

#define SPI_MOSI_PIN 13
#define SPI_MISO_PIN 12
#define SPI_CLK_PIN 14

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define I2C_FREQUENCY_HZ 400000
#define I2C_PORT 0

#define ADC_RESOLUTION 10

#define CANBUS_INTERRUPT_PIN GPIO_NUM_4
#define CANBUS_CHIP_SELECT_PIN 15

#define BLUETOOTH_ENABLE false
#define WIFI_ENABLE false

#define DISPLAY_I2C_ADDRESS 0x3C
#define DISPLAY_I2C_TIMEOUT_MS 1000
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_UPSIDE_DOWN false
#define DISPLAY_ERROR_MESSAGE_TIME_MS 7000
#define DISPLAY_ERROR_MESSAGE_MAX_LENGTH ((DISPLAY_WIDTH - 12) / 5)

#define CRUISE_CONTROL_MAX_RPM_LIMIT 4500
#define CRUISE_CONTROL_PID_ITERATION_TIME 100
#define CRUISE_CONTROL_PID_Kp 0.03
#define CRUISE_CONTROL_PID_Ki 0.000010
#define CRUISE_CONTROL_PID_Kd 0.0

#define CAR_GAS_PEDAL_RESOLUTION ADC_RESOLUTION
#define CAR_GAS_PEDAL_ADC_CHANNEL_0 ADC1_CHANNEL_0    // Mapped to PIN 36
#define CAR_GAS_PEDAL_ADC_CHANNEL_1 ADC1_CHANNEL_1    // Mapped to PIN 37
#define CAR_GAS_PEDAL_ADC_SAMPLE_COUNT 100
#define CAR_GAS_PEDAL_MIN_VALUE 38  // = 0.38 / 5 * 1023 / 2
#define CAR_GAS_PEDAL_MAX_VALUE 865 // = 4.23 / 5 * 1023
#define CAR_VIRTUAL_GAS_PEDAL_OUTPUT_PIN_0 GPIO_NUM_32
#define CAR_VIRTUAL_GAS_PEDAL_OUTPUT_PIN_1 GPIO_NUM_32
#define CAR_VIRTUAL_GAS_PEDAL_TIMER LEDC_TIMER_0
#define CAR_VIRTUAL_GAS_PEDAL_TIMER_SPEED_MODE LEDC_HIGH_SPEED_MODE
#define CAR_VIRTUAL_GAS_PEDAL_TIMER_CHANNEL_0 LEDC_CHANNEL_0
#define CAR_VIRTUAL_GAS_PEDAL_TIMER_CHANNEL_1 LEDC_CHANNEL_0

#define BUTTONS_ADC_CHANNEL_0 ADC1_CHANNEL_2    // Mapped to PIN 38
#define BUTTONS_ADC_CHANNEL_1 ADC1_CHANNEL_3    // Mapped to PIN 39
#define BUTTON_UPPER_LIMIT ((int) (798 + (985 - 798) / 2))
#define BUTTON_MIDDLE_LIMIT ((int) (510 + (798 - 510) / 2))
#define BUTTON_LOWER_LIMIT ((int) (510 / 2))
#define BUTTONS_READ_INTERVAL_LOOPS 40          // Only read the buttons once very X loops, to decrease the total time this takes
#define BUTTON_AVERAGE_READ_SAMPLES 1
#define BUTTON_MIN_PRESS_TIME_MS 80             // Minimum time the button must be pressed for it to register a valid press (ms)
#define BUTTON_LONG_PRESS_MS 2000
#define BUTTON_DEBOUNCE_COOLDOWN_PERIOD_MS 80   // Don't check the button after is has been pressed for this amount of time (ms)

#ifndef DEFAULT_SSID
#define DEFAULT_SSID "abc"
#endif
#ifndef DEFAULT_PWD
#define DEFAULT_PWD "***"
#endif

#ifdef __cplusplus
}
#endif

#endif //APP_TEMPLATE_CONFIG_H
