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

int64_t esp_timer_get_time_ms();

void delay_ms(unsigned long ms);

void utils_reboot(State *state);

double average_read_channel(adc1_channel_t channel, int sample_count);

uint8_t starts_with(const char *source, const char *needle);

void string_char_replace(char *source, char needle, char replacement);

void string_char_remove(char **source, char needle);

void string_escape(const char *input, char **destination);

void set_error(State *state, uint32_t error_code);

#endif //APP_TEMPLATE_UTILS_H
