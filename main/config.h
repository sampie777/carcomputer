//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_CONFIG_H
#define APP_TEMPLATE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "secrets.h"

#define SPI_MOSI_PIN 13
#define SPI_MISO_PIN 12
#define SPI_CLK_PIN 14

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define I2C_FREQUENCY 400000

#define CANBUS_INTERRUPT_PIN GPIO_NUM_4
#define CANBUS_CHIP_SELECT_PIN 15

#define DISPLAY_I2C_ADDRESS 0x3C
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_UPSIDE_DOWN false
#define DISPLAY_ERROR_MESSAGE_TIME 7000
#define DISPLAY_ERROR_MESSAGE_MAX_LENGTH (DISPLAY_WIDTH / 8)  // = 16

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
