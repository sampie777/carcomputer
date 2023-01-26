//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_CONFIG_H
#define APP_TEMPLATE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "secrets.h"

// Unused GPIOs: 22 (GPIO15 will/must be HIGH on device reset: strapping pin)
// GPIO16-17 are usually connected to the SPI flash and PSRAM integrated on the module
// and therefore should not be used for other purposes (https://docs.espressif.com/projects/esp-idf/en/v4.4.1/esp32/api-reference/peripherals/gpio.html)

// Features
#define POWER_OFF_ENABLE false
#define BLUETOOTH_ENABLE false
#define WIFI_ENABLE false
#define GSM_ENABLE true
#define SD_ENABLE true
#define CRUISE_CONTROL_ENABLE false

// Detailed feature settings
#define POWER_OFF_MAX_TIMEOUT_MS 90000
#define POWER_PIN GPIO_NUM_23

#define WIFI_SCAN_INTERVAL_MS 2500
#define WIFI_SCAN_MAX_DURATION 8000
#define WIFI_SCAN_MAX_APS 16

#define BACKEND_REGISTRATION_TOKEN_LENGTH 7
#define BACKEND_REGISTRATION_STATUS_URL "https://car.sajansen.nl/api/v1/devices/register"

#define DATA_UPLOAD_URL "http://car.sajansen.nl/api/v1/cars/logs"
#define TRIP_LOGGER_ENGINE_OFF_GRACE_TIME_MS 10000
#define TRIP_LOGGER_UPLOAD_RETRY_TIMEOUT_MS 3000
#define TRIP_LOGGER_UPLOAD_URL_TRIP_END DATA_UPLOAD_URL
#define DATA_LOGGER_LOG_INTERVAL_MS 500
#define DATA_LOGGER_MINIMAL_DATA_LOG_INTERVAL_MS 5000
#define DATA_LOGGER_MINIMAL_DATA_UPLOAD_INTERVAL_MS 20000
#define DATA_LOGGER_UPLOAD_URL_LOG_INTERVAL DATA_UPLOAD_URL   // Comment to disable interval upload
#define DATA_LOGGER_UPLOAD_URL_FULL_DATA DATA_UPLOAD_URL

#define SPI_MOSI_PIN GPIO_NUM_12
#define SPI_MISO_PIN GPIO_NUM_13
#define SPI_CLK_PIN GPIO_NUM_14
#define SPI_DEFAULT_HOST HSPI_HOST

#define I2C_FREQUENCY_HZ 400000
#define I2C_TIMEOUT_MS 1000

#define MAIN_I2C_PORT I2C_NUM_0
#define MAIN_I2C_SDA_PIN GPIO_NUM_18
#define MAIN_I2C_SCL_PIN GPIO_NUM_5

#define ADC_RESOLUTION 10

#define CANBUS_INTERRUPT_PIN GPIO_NUM_4
#define CANBUS_CHIP_SELECT_PIN GPIO_NUM_2

#define DISPLAY_I2C_PORT I2C_NUM_1
#define DISPLAY_I2C_SDA_PIN GPIO_NUM_21
#define DISPLAY_I2C_SCL_PIN GPIO_NUM_19
#define DISPLAY_I2C_ADDRESS 0x3C
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_UPSIDE_DOWN true
#define DISPLAY_UPDATE_MIN_INTERVAL 40  // 25 fps
#define DISPLAY_ERROR_MESSAGE_TIME_MS 2000
#define DISPLAY_LONG_BLINK_INTERVAL 1000

#define CRUISE_CONTROL_MAX_RPM_LIMIT 4500
#define CRUISE_CONTROL_PID_ITERATION_TIME 100
#define CRUISE_CONTROL_PID_Kp 0.03
#define CRUISE_CONTROL_PID_Ki 0.000010
#define CRUISE_CONTROL_PID_Kd 0.0

#define CAR_CAN_MAX_MESSAGE_RECEIVE_TIMEOUT 1000
#define CAR_CAN_CONTROLLER_CHECK_INTERVAL 500

#define CAR_GAS_PEDAL_RESOLUTION ADC_RESOLUTION
#define CAR_GAS_PEDAL_ADC_CHANNEL_0 ADC1_CHANNEL_6    // Mapped to PIN 34
#define CAR_GAS_PEDAL_ADC_CHANNEL_1 ADC1_CHANNEL_7    // Mapped to PIN 35
#define CAR_GAS_PEDAL_ADC_SAMPLE_COUNT 20
#define CAR_GAS_PEDAL_MIN_VALUE 38  // = 0.38 / 5 * 1023 / 2
#define CAR_GAS_PEDAL_MAX_VALUE 865 // = 4.23 / 5 * 1023

#define CAR_VIRTUAL_GAS_PEDAL_OUTPUT_PIN_0 GPIO_NUM_32
#define CAR_VIRTUAL_GAS_PEDAL_OUTPUT_PIN_1 GPIO_NUM_33
#define CAR_VIRTUAL_GAS_PEDAL_ENABLE_PIN GPIO_NUM_27
#define CAR_VIRTUAL_GAS_PEDAL_TIMER LEDC_TIMER_0
#define CAR_VIRTUAL_GAS_PEDAL_TIMER_SPEED_MODE LEDC_HIGH_SPEED_MODE
#define CAR_VIRTUAL_GAS_PEDAL_TIMER_CHANNEL_0 LEDC_CHANNEL_0
#define CAR_VIRTUAL_GAS_PEDAL_TIMER_CHANNEL_1 LEDC_CHANNEL_1
#define CAR_VIRTUAL_GAS_PEDAL_RISE_TIME_MS 100  // Time to wait before switching gas pedal to virtual gas pedal

#define CAR_CLAXON_PIN GPIO_NUM_25
#define CAR_ENGINE_SHUTOFF_DISABLE_PIN GPIO_NUM_26

// Gear ration is calculated by speed / rpm
//#define CAR_GEAR_1_RATIO 0.00586
//#define CAR_GEAR_1_RATIO 0.00605
//#define CAR_GEAR_1_RATIO 0.00619
//#define CAR_GEAR_1_RATIO 0.00603  // Average
#define CAR_GEAR_1_RATIO 6    // Average rounded
//#define CAR_GEAR_2_RATIO 0.01203
//#define CAR_GEAR_2_RATIO 0.01186
//#define CAR_GEAR_2_RATIO 0.01193
//#define CAR_GEAR_2_RATIO 0.01194  // Average
#define CAR_GEAR_2_RATIO 11    // Average rounded
//#define CAR_GEAR_3_RATIO 0.01722
//#define CAR_GEAR_3_RATIO 0.01728
//#define CAR_GEAR_3_RATIO 0.01734
//#define CAR_GEAR_3_RATIO 0.01728  // Average
#define CAR_GEAR_3_RATIO 17    // Average rounded
//#define CAR_GEAR_4_RATIO 0.02389
//#define CAR_GEAR_4_RATIO 0.02387
//#define CAR_GEAR_4_RATIO 0.02394
//#define CAR_GEAR_4_RATIO 0.02390  // Average
#define CAR_GEAR_4_RATIO 23    // Average rounded
//#define CAR_GEAR_5_RATIO 0.02904
//#define CAR_GEAR_5_RATIO 0.02940
//#define CAR_GEAR_5_RATIO 0.02949
//#define CAR_GEAR_5_RATIO 0.02931  // Average
#define CAR_GEAR_5_RATIO 29    // Average rounded

#define BUTTONS_ADC_CHANNEL_0 ADC1_CHANNEL_0    // Mapped to PIN 36
#define BUTTONS_ADC_CHANNEL_1 ADC1_CHANNEL_3    // Mapped to PIN 39
#define BUTTON_UPPER_LIMIT ((int) (798 + (985 - 798) / 2))
#define BUTTON_MIDDLE_LIMIT ((int) (510 + (798 - 510) / 2))
#define BUTTON_LOWER_LIMIT ((int) (510 / 2))
#define BUTTONS_READ_INTERVAL_MS 67          // Only read the buttons once very X loops, to decrease the total time this takes
#define BUTTON_AVERAGE_READ_SAMPLES 5
#define BUTTON_MIN_PRESS_TIME_MS 80             // Minimum time the button must be pressed for it to register a valid press (ms)
#define BUTTON_LONG_PRESS_MS 2000
#define BUTTON_DEBOUNCE_COOLDOWN_PERIOD_MS 80   // Don't check the button after is has been pressed for this amount of time (ms)

#define MOTION_SENSOR_I2C_ADDRESS 0x68
#define COMPASS_SENSOR_I2C_ADDRESS 0x0C
#define MOTION_SENSOR_ROOM_TEMPERATURE_OFFSET 112
#define MOTION_SENSOR_TEMPERATURE_SENSITIVITY 42.85
#define MOTION_SENSOR_READ_INTERVAL_MS 20

#define SD_CHIP_SELECT_PIN GPIO_NUM_15

#define GPSGSM_UART_NUMBER UART_NUM_2
#define GPSGSM_UART_TX_PIN GPIO_NUM_17
#define GPSGSM_UART_RX_PIN GPIO_NUM_16
#define GPSGSM_UART_BAUD_RATE 115200
#define GPSGSM_INIT_MAX_TIMEOUT_MS 20000
#define GPSGSM_MESSAGE_MAX_TIMEOUT_MS 20000
#define GPSGSM_SMS_SENT_MAX_TIMEOUT_MS 20000

#define CRASH_DETECTION_CRASH_MIN_G 30
#define CRASH_DETECTION_CRASH_MAX_DURATION_MS 30000

#ifndef DEFAULT_SSID
#define DEFAULT_SSID "abc"
#endif
#ifndef DEFAULT_PWD
#define DEFAULT_PWD "***"
#endif
#ifndef DEFAULT_SSID1
#define DEFAULT_SSID1 "abc1"
#endif
#ifndef DEFAULT_PWD1
#define DEFAULT_PWD1 "***"
#endif

#ifdef __cplusplus
}
#endif

#endif //APP_TEMPLATE_CONFIG_H
