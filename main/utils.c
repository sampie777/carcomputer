//
// Created by samuel on 17-7-22.
//

#include <esp_timer.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "utils.h"

int64_t esp_timer_get_time_ms() {
    return esp_timer_get_time() / 1000;
}

void delay_ms(unsigned long ms) {
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

void utils_reboot(State *state) {
    state->is_rebooting = true;
    delay_ms(2000);
    esp_restart();
}

double average_read_channel(adc1_channel_t channel, int sample_count) {
    double total = 0;
    for (int i = 0; i < sample_count; i++) {
        total += adc1_get_raw(channel);
    }
    return total / sample_count;
}

uint8_t starts_with(const char *source, const char *needle) {
    return strncmp(needle, source, strlen(needle)) == 0;
}

void string_char_replace(char *source, char needle, char replacement) {
    for (int i = 0; i < strlen(source); i++) {
        if (source[i] != needle) continue;
        source[i] = replacement;
    }
}

void string_char_remove(char **source, char needle) {
    char *out = malloc(strlen(*source) + 1);
    int out_size = 0;

    // Copy over string without needle
    for (int i = 0; i < strlen(*source) + 1; i++) {
        if ((*source)[i] == needle) continue;
        out[out_size++] = (*source)[i];
    }

    // Put new value back into original string
    *source = malloc(out_size + 1);
    strcpy(*source, out);
    free(out);
}

void string_escape(const char *input, char **destination) {
    size_t escaped_size = 0;
    size_t input_size = strlen(input);
    *destination = malloc(input_size + 1);

    for (int i = 0; i < input_size; i++) {
        if (input[i] == '"') {
            *destination = realloc(*destination, input_size + escaped_size + 2);
            (*destination)[i + escaped_size] = '\\';
            escaped_size++;
        }
        (*destination)[i + escaped_size] = input[i];
    }
    (*destination)[input_size + escaped_size] = '\0';
}

void set_error(State *state, uint32_t error_code) {
    static uint32_t previous_errors = 0;
    state->errors |= error_code;

    if (state->errors != previous_errors) {
        // Only print this once
        printf("Set error code: %u\n", error_code);
        previous_errors = state->errors;
    }
}
