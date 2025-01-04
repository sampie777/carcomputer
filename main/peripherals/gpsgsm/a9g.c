//
// Created by samuel on 2025/01/03.
//

#include "a9g.h"
#include "../../config.h"
#include "../../utils.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <string.h>
#include <os/os.h>

#include "gpsgsm.h"

#define MESSAGE_MAX_LENGTH (A9G_UART_BUFFER_SIZE)
#define MESSAGE_LOG_MAX_LENGTH 32

char message_log[MESSAGE_LOG_MAX_LENGTH][MESSAGE_MAX_LENGTH];
int message_log_length = 0;

void debug_print_message_log() {
    if (message_log_length == 0) {
        printf("[GPS] No messages in log\n");
        return;
    }
    for (int i = 0; i < message_log_length; i++) {
        printf("[GPS] Log: [%d] '%s' %d\n", i, message_log[i], strlen(message_log[i]));
    }
}

void a9g_log_message(const char* message) {
    if (message_log_length >= MESSAGE_LOG_MAX_LENGTH) {
        // Shift all messages one up
        for (int i = 0; i < MESSAGE_LOG_MAX_LENGTH - 1; i++) {
            strcpy(message_log[i], message_log[i + 1]);
        }
        message_log_length = MESSAGE_LOG_MAX_LENGTH - 1;
    }

    strcpy(message_log[message_log_length++], message);
}

void a9g_transmit(const char* data, uint8_t with_break) {
    char* buffer = malloc(strlen(data) + 2);
    sprintf(buffer, "%s\r", data);

    if (with_break) {
        uart_write_bytes_with_break(GPSGSM_UART_NUMBER, buffer, strlen(buffer), 50);
    } else {
        uart_write_bytes(GPSGSM_UART_NUMBER, buffer, strlen(buffer));
    }

    sprintf(buffer, ">%s", data);
    a9g_log_message(buffer);
    free(buffer);
}

void a9g_reset() {
    printf("[GPS] Reset A9G chip\n");
    a9g_transmit(A9G_RESET, true);
}

void a9g_receive() {
    static char* last_message = NULL;
    static int last_message_index = 0;

    // Initialize static pointer
    if (last_message == NULL) {
        last_message = malloc(MESSAGE_MAX_LENGTH);
        bzero(last_message, MESSAGE_MAX_LENGTH); // Clear buffer
    }

    // Check if UART has data available
    uart_event_t event;
    if (!xQueueReceive(uart_queue, &event, 0)) return;

    // Read data from UART
    char* data = malloc(A9G_UART_BUFFER_SIZE);
    switch (event.type) {
        case UART_DATA:
            bzero(data, A9G_UART_BUFFER_SIZE); // Clear buffer
            uart_read_bytes(GPSGSM_UART_NUMBER, data, event.size, portMAX_DELAY);
        // printf("[uart] Received data: %s with length %d / %d\n", data, event.size, strlen(data));
            break;
        default:
            printf("[uart] *unhandled event: %d\n", event.type);
            printf("[uart] unhandled event: %d\n", event.type);
            printf("[uart] unhandled event: %d\n", event.type);
            printf("[uart] unhandled event: %d\n", event.type);
            printf("[uart] unhandled event: %d*\n", event.type);
            return;
    }

    // Process data

    if (event.size > strlen(A9G_INIT) && strstr(data, A9G_INIT) != NULL) {
        message_log_length = 0; // Start with a new log as all previous commands are reset
    }

    // Use event.size as the data may contain \0 characters
    for (int i = 0; i < event.size; i++) {
        if (data[i] == '\0') continue;

        if (last_message_index >= MESSAGE_MAX_LENGTH) {
            printf("[GPS] Max message length reached\n");
            last_message[MESSAGE_MAX_LENGTH - 1] = '\0';

            string_strip_char(&last_message, '\n');
            a9g_log_message(last_message);

            // Clear buffer
            last_message_index = 0;
            bzero(last_message, MESSAGE_MAX_LENGTH);
            continue;
        }

        if (data[i] == '\r') {
            string_strip_char(&last_message, '\n');
            if (strlen(last_message) > 0) {
                a9g_log_message(last_message);
            }

            // Clear buffer
            last_message_index = 0;
            bzero(last_message, MESSAGE_MAX_LENGTH);
            continue;
        }

        last_message[last_message_index++] = data[i];
    }

    free(data);
}

int message_logs_contains(const char* message) {
    for (int i = message_log_length - 1; i >= 0; i--) {
        if (strcmp(message_log[i], message) == 0) {
            return i;
        }
    }
    return -1;
}

int message_logs_contains_transmitted(const char* message) {
    char* buffer = malloc(strlen(message) + 2);
    sprintf(buffer, ">%s", message);

    int result = message_logs_contains(buffer);

    free(buffer);
    return result;
}

bool a9g_check_if_init_done() {
    // Check if Init message is found in the messages logs
    return message_logs_contains("+CREG: 3") >= 0;
}

bool a9g_send_and_wait_for_command(const char* command) {
    // Check if we need to send the command
    int command_sent_index = message_logs_contains_transmitted(command);

    if (command_sent_index < 0) {
        printf("Command %s not send, sending now\n", command);
        a9g_transmit(command, true);
        return false;
    }

    // Check if we got a response already
    int command_received_index = message_logs_contains(command);

    printf("%d / %d\t", command_received_index, message_log_length);
    if (command_received_index < 0 || message_log_length <= command_received_index + 1) {
        printf("Command %s not received\n", command);
        return false;
    }

    bool command_is_ok = strcmp(message_log[command_received_index + 1], "OK") == 0;
    if (command_is_ok) {
        printf("Command %s is OK\n", command);
    } else {
        printf("Command %s is not OK: %d '%s'\n", command, command_received_index,
               message_log[command_received_index + 1]);
    }
    return command_is_ok;
}

bool a9g_check_if_network_attached(const bool force) {
    static bool init_finished = false;
    if (force || !init_finished) {
        init_finished = a9g_send_and_wait_for_command(A9G_CGATT_ENABLE);
    }
    return init_finished;
}

bool a9g_check_if_pnp_parameters_set(const bool force) {
    static bool init_finished = false;
    if (force || !init_finished) {
        init_finished = a9g_send_and_wait_for_command(A9G_CGDCONT_ENABLE);
    }
    return init_finished;
}

bool a9g_check_if_pnp_activated(const bool force) {
    static bool init_finished = false;
    if (force || !init_finished) {
        init_finished = a9g_send_and_wait_for_command(A9G_CGACT_PNP_ENABLE);
    }
    return init_finished;
}

bool a9g_check_if_agps_enabled(const bool force) {
    static bool init_finished = false;
    if (force || !init_finished) {
        init_finished = a9g_send_and_wait_for_command(A9G_AGPS_ENABLE);
    }
    return init_finished;
}

bool a9g_check_if_gps_enabled(const bool force) {
    static bool init_finished = false;
    if (force || !init_finished) {
        init_finished = a9g_send_and_wait_for_command(A9G_GPS_ENABLE);
    }
    return init_finished;
}

bool a9g_check_if_gps_logging_enabled(const bool force) {
    static bool init_finished = false;
    if (force || !init_finished) {
        init_finished = a9g_send_and_wait_for_command(A9G_GPSRD_ENABLE);
    }
    return init_finished;
}

void a9g_proceed_device_init(State* state) {
    if (message_log_length == 0) return;

    if (!a9g_check_if_init_done()) return;
    // if (!a9g_check_if_network_attached(false)) return;
    // if (!a9g_check_if_pnp_parameters_set(false)) return;
    // if (!a9g_check_if_pnp_activated(false)) return;
    // if (!a9g_check_if_agps_enabled(false)) return;
    if (!a9g_check_if_gps_enabled(false)) return;
    if (!a9g_check_if_gps_logging_enabled(false)) return;
}

void a9g_process_messages(State* state) {
    bool process_gngga_message_done = false;
    bool process_gnrmc_message_done = false;
    bool process_ctzv_message_done = false;

    for (int i = message_log_length - 1; i >= 0; i--) {
        if (!process_gngga_message_done && starts_with(message_log[i], "$GNGGA")) {
            state->location.is_gps_on = true;
            process_gngga_message(state, message_log[i]);
            process_gngga_message_done = true;
            continue;
        }
        if (!process_gnrmc_message_done && starts_with(message_log[i], "$GNRMC")) {
            state->location.is_gps_on = true;
            process_gnrmc_message(state, message_log[i]);
            process_gnrmc_message_done = true;
            continue;
        }
        if (!process_ctzv_message_done && starts_with(message_log[i], "+CTZV:")) {
            process_ctzv_message(state, message_log[i]);
            process_ctzv_message_done = true;
            continue;
        }
    }
}

void a9g_validate_location_data(State* state) {
    // Reset location data after it becomes invalid (expires)
    if (esp_timer_get_time_ms() > state->location.gngga_last_updated + GPSGSM_LOCATION_MAX_VALID_TIME_MS) {
        state->location.latitude = 0;
        state->location.longitude = 0;
        state->location.altitude = 0;
        state->location.quality = 0;
        state->location.satellites = 0;

        state->location.time.minutes = 0;
        state->location.time.seconds = 0;
        state->location.time.hours = 0;
    }

    if (esp_timer_get_time_ms() > state->location.gnrmc_last_updated + GPSGSM_LOCATION_MAX_VALID_TIME_MS) {
        state->location.is_effective_positioning = false;
        state->location.ground_speed = 0;
        state->location.ground_heading = 0;
    }
}

void a9g_process(State* state) {
    a9g_receive();

    update_time(state);
    a9g_validate_location_data(state);

    if (message_log_length == 0) return;

    a9g_proceed_device_init(state);
    a9g_process_messages(state);
}

void a9g_init(State* state) {
    gpsgsm_init(&state->a9g);
}
