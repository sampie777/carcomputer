//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_CONFIG_H
#define APP_TEMPLATE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "secrets.h"

#define CANBUS_INTERRUPT_PIN GPIO_NUM_4
#define CANBUS_CHIP_SELECT_PIN 15

#define DISPLAY_ERROR_MESSAGE_TIME 7000
#define DISPLAY_ERROR_MESSAGE_MAX_LENGTH 32

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
