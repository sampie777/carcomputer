//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_UTILS_H
#define APP_TEMPLATE_UTILS_H

#include "state.h"

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

void nvs_init();

unsigned long esp_timer_get_time_ms();

void utils_reboot(State *state);

#endif //APP_TEMPLATE_UTILS_H
