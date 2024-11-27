//
// Created by samuel on 17-7-22.
//

#include <esp_timer.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "utils.h"

#include "error_codes.h"

int64_t esp_timer_get_time_ms() {
    return esp_timer_get_time() / 1000;
}

void delay_ms(unsigned long ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void utils_reboot(State *state) {
    state->is_rebooting = true;
    delay_ms(2000);
    esp_restart();
}

double average_read_channel(adc1_channel_t channel, int sample_count) {
    double total = 0;
    sample_count = max(1, sample_count);
    for (int i = 0; i < sample_count; i++) {
        total += adc1_get_raw(channel);

        // Add a bit of delay (10 clock cycles) to get a new sample
        for (volatile int i = 0; i < 10; i++) {
        }
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
        printf("Set error code: ");
        switch (error_code) {
            case ERROR_PEDAL_DISCONNECTED: printf("ERROR_PEDAL_DISCONNECTED");
                break;
            case ERROR_SPI_FAILED: printf("ERROR_SPI_FAILED");
                break;
            case ERROR_CRASH_NO_ICE: printf("ERROR_CRASH_NO_ICE");
                break;
            case ERROR_GPS_TIMEOUT: printf("ERROR_GPS_TIMEOUT");
                break;
            case ERROR_SMS_FAILED: printf("ERROR_SMS_FAILED");
                break;
            case ERROR_SD_FULL: printf("ERROR_SD_FULL");
                break;
            case ERROR_CRASH_DETECTED: printf("ERROR_CRASH_DETECTED");
                break;
            default: printf("unknown");
        }
        printf("\n");
        previous_errors = state->errors;
    }
}

void invert_array(const uint8_t *array, uint8_t *output_array, int length) {
    for (int i = 0; i < length; i++) {
        output_array[i] = array[length - i - 1];
    }
}

void convert_to_base_26(uint32_t input, char *output, size_t max_length) {
    memset(output, '\0', max_length);

    uint32_t temp = input;
    int i = 0;

    while (temp > 0 && i <= max_length - 1) {
        uint32_t digit_value = temp % 26;
        temp = temp / 26;
        // Start char at ascii A
        output[i++] = (char) (digit_value + 65);
    }
}

double scale(double value, double min, double max) {
    double difference = max - min;
    return min + difference * value;
}

void debug_state(const State *state) {
    printf("State:\n");

    printf("\tis_booting: %c\n", state->is_booting ? 'y' : 'n');
    printf("\tis_rebooting: %c\n", state->is_rebooting ? 'y' : 'n');
    printf("\tpower_off_count_down_sec: %hd\n", state->power_off_count_down_sec);
    printf("\tlogging_session_id: %lu\n", state->logging_session_id);
    printf("\terrors: %lu\n", state->errors);

    printf("\tcar:\n");
    printf("\t\tis_connected: %c\n", state->car.is_connected ? 'y' : 'n');
    printf("\t\tis_controller_connected: %c\n", state->car.is_controller_connected ? 'y' : 'n');
    printf("\t\tis_braking: %c\n", state->car.is_braking ? 'y' : 'n');
    printf("\t\tis_ignition_on: %c\n", state->car.is_ignition_on ? 'y' : 'n');
    printf("\t\tis_in_reverse: %c\n", state->car.is_in_reverse ? 'y' : 'n');
    printf("\t\tspeed: %lf\n", state->car.speed);
    printf("\t\trpm: %lf\n", state->car.rpm);
    printf("\t\trpm_raw: %hu\n", state->car.rpm_raw);
    printf("\t\tlast_can_message_time: %lld\n", state->car.last_can_message_time);
    printf("\t\todometer_start: %lu\n", state->car.odometer_start);
    printf("\t\todometer: %lu\n", state->car.odometer);
    printf("\t\testimated_gear: %d\n", state->car.estimated_gear);
    printf("\t\tgas_pedal_connected: %c\n", state->car.gas_pedal_connected ? 'y' : 'n');
    printf("\t\tgas_pedal_0_min_value_volts: %lf\n", state->car.gas_pedal_0_min_value_volts);
    printf("\t\tgas_pedal_1_min_value_volts: %lf\n", state->car.gas_pedal_1_min_value_volts);
    printf("\t\tgas_pedal_0_max_value_volts: %lf\n", state->car.gas_pedal_0_max_value_volts);
    printf("\t\tgas_pedal_1_max_value_volts: %lf\n", state->car.gas_pedal_1_max_value_volts);
    printf("\t\tgas_pedal: %lf\n", state->car.gas_pedal);
    printf("\t\tgas_pedal_0_volts: %lf\n", state->car.gas_pedal_0_volts);
    printf("\t\tgas_pedal_1_volts: %lf\n", state->car.gas_pedal_1_volts);
    printf("\t\tis_drivers_door_open: %c\n", state->car.is_drivers_door_open ? 'y' : 'n');
    printf("\t\tis_other_doors_open: %c\n", state->car.is_other_doors_open ? 'y' : 'n');
    printf("\t\tis_blower_on: %c\n", state->car.is_blower_on ? 'y' : 'n');
    printf("\t\tis_locked: %c\n", state->car.is_locked ? 'y' : 'n');

    printf("\tcruise_control:\n");
    printf("\t\tenabled: %c\n", state->cruise_control.enabled ? 'y' : 'n');
    printf("\t\ttarget_speed: %lf\n", state->cruise_control.target_speed);
    printf("\t\tvirtual_gas_pedal: %lf\n", state->cruise_control.virtual_gas_pedal);
    printf("\t\tinitial_control_value: %lf\n", state->cruise_control.initial_control_value);
    printf("\t\tcontrol_value: %lf\n", state->cruise_control.control_value);
    printf("\t\tpidKp: %lf\n", state->cruise_control.pidKp);
    printf("\t\tpidKi: %lf\n", state->cruise_control.pidKi);
    printf("\t\tpidKd: %lf\n", state->cruise_control.pidKd);
}

void wdt_feed(int max_timeout_ms) {
    static int64_t last_fed = 0;
    // Check time against the max timeout minus a safety margin to not make it absolutely last minute
    if (last_fed != 0 && esp_timer_get_time_ms() < last_fed + max(100, max_timeout_ms - 1000)) return;
    last_fed = esp_timer_get_time_ms();
    vTaskDelay(1);
}
