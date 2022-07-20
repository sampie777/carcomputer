//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_CONFIG_H
#define APP_TEMPLATE_CONFIG_H

#include "secrets.h"

#define CANBUS_INTERRUPT_PIN GPIO_NUM_4
#define CANBUS_CHIP_SELECT_PIN GPIO_NUM_5

#ifndef DEFAULT_SSID
#define DEFAULT_SSID "abc"
#endif
#ifndef DEFAULT_PWD
#define DEFAULT_PWD "***"
#endif

#endif //APP_TEMPLATE_CONFIG_H
