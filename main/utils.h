//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_UTILS_H
#define APP_TEMPLATE_UTILS_H

#include "state.h"
#include <driver/adc.h>
#include <string.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

void nvs_init();

int64_t esp_timer_get_time_ms();

void delay_ms(unsigned long ms);

void utils_reboot(State *state);

double average_read_channel(adc1_channel_t channel, int sample_count);

char *string_remove_chars(const char *string, char needle);

#endif //APP_TEMPLATE_UTILS_H
