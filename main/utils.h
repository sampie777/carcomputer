//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_UTILS_H
#define APP_TEMPLATE_UTILS_H

#include "state.h"
#include <string.h>

int64_t esp_timer_get_time_ms();
void delay_ms(unsigned long ms);
void utils_reboot(State *state);
uint8_t starts_with(const char *source, const char *needle);
void string_char_replace(char *source, char needle, char replacement);
void string_char_remove(char **source, char needle);
void string_strip_char(char **input, char needle);
void string_escape(const char *input, char **destination);
void set_error(State *state, uint32_t error_code);
void reset_error(State *state, uint32_t error_code);
void invert_array(const uint8_t *array, uint8_t *output_array, int length);
void convert_to_base_26(uint32_t input, char *output, size_t max_length);
double scale(double value, double min, double max);
void debug_state(const State *state);
void wdt_feed(int max_timeout_ms);
void format_time(int64_t milliseconds, char *output_string);
void format_time_h_mm(int64_t milliseconds, char *output_string);

#endif //APP_TEMPLATE_UTILS_H
