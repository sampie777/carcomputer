//
// Created by samuel on 2025/01/03.
//

#include "a9g.h"
#include "../../utils.h"
#include "../../error_codes.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <string.h>

#include "gpsgsm.h"
#include "utils.h"

#define MESSAGE_MAX_LENGTH (A9G_UART_BUFFER_SIZE)
#define MESSAGE_LOG_MAX_LENGTH 32

char message_log[MESSAGE_LOG_MAX_LENGTH][MESSAGE_MAX_LENGTH];
int64_t message_log_timestamps[MESSAGE_LOG_MAX_LENGTH];
int message_log_length = 0;

void debug_print_message_log() {
    if (message_log_length == 0) {
        printf("DEBUG [GPS] No messages in log\n");
        return;
    }
    int i = message_log_length - 1;
//    for (int i = 0; i < message_log_length; i++) {
        printf("DEBUG [GPS] Log: %d [%lld] '%s' %d\n", i, message_log_timestamps[i], message_log[i],
               strlen(message_log[i]));
//    }
}

void a9g_log_message(const char *message) {
    // Ignore battery messages
    if (starts_with(message, "+CIEV")) return;

    if (message_log_length >= MESSAGE_LOG_MAX_LENGTH) {
        // Shift all messages one up
        for (int i = 0; i < MESSAGE_LOG_MAX_LENGTH - 1; i++) {
            strcpy(message_log[i], message_log[i + 1]);
            message_log_timestamps[i] = message_log_timestamps[i + 1];
        }
        message_log_length = MESSAGE_LOG_MAX_LENGTH - 1;
    }

    strcpy(message_log[message_log_length], message);
    message_log_timestamps[message_log_length] = esp_timer_get_time_ms();
    message_log_length++;

//    debug_print_message_log();
}

void a9g_transmit(const char *data, uint8_t with_break) {
    char *buffer = malloc(strlen(data) + 2);
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

void a9g_reset(State *state) {
    printf("[GPS] Reset A9G chip\n");
    a9g_transmit(A9G_RESET, true);
    state->location.is_gps_on = false;
    a9g_state_reset(&state->a9g);
}

void a9g_receive(A9GState *state) {
    static char *last_message = NULL;
    static int last_message_index = 0;

    // Initialize static pointer
    if (last_message == NULL) {
        last_message = malloc(MESSAGE_MAX_LENGTH);
        memset(last_message, 0, MESSAGE_MAX_LENGTH);  // Clear buffer
    }

    // Check if UART has data available
    uart_event_t event;
    if (!xQueueReceive(uart_queue, &event, 0)) return;

    // Read data from UART
    char *data = malloc(A9G_UART_BUFFER_SIZE);
    switch (event.type) {
        case UART_DATA:
            memset(data, 0, A9G_UART_BUFFER_SIZE);  // Clear buffer
            uart_read_bytes(GPSGSM_UART_NUMBER, data, event.size, portMAX_DELAY);
            // printf("[uart] Received data: %s with length %d / %d\n", data, event.size, strlen(data));
            break;
        case UART_BREAK:
            break;
        default:
            printf("[uart] unhandled event: %d %s\n", event.type, uart_type_to_string(event.type));
            return;
    }

    // Process data

    if (event.size > strlen(A9G_INIT) && strstr(data, A9G_INIT) != NULL) {
        message_log_length = 0; // Start with a new log as all previous commands are reset
        a9g_state_reset(state);
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
            memset(last_message, 0, MESSAGE_MAX_LENGTH);
            continue;
        }

        if (data[i] == '\r') {
            string_strip_char(&last_message, '\n');
            if (strlen(last_message) > 0) {
                a9g_log_message(last_message);
            }

            // Clear buffer
            last_message_index = 0;
            memset(last_message, 0, MESSAGE_MAX_LENGTH);
            continue;
        }

        last_message[last_message_index++] = data[i];
    }

    free(data);
}

int message_logs_contains(const char *message, const bool exact_match) {
    for (int i = message_log_length - 1; i >= 0; i--) {
        if (exact_match && strcmp(message_log[i], message) == 0) {
            return i;
        }
        if (!exact_match && starts_with(message_log[i], message)) {
            return i;
        }
    }
    return -1;
}

int message_logs_contains_exact(const char *message) {
    return message_logs_contains(message, true);
}

int message_logs_contains_transmitted(const char *message) {
    char *buffer = malloc(strlen(message) + 2);
    sprintf(buffer, ">%s", message);

    int result = message_logs_contains_exact(buffer);

    free(buffer);
    return result;
}

bool a9g_check_if_init_done(A9GState *state) {
    if (state->initialized) return true;

    // Check if Init message is found in the messages logs
    int init_message_index = message_logs_contains_exact(A9G_INIT);
    int ready_message_index = max(message_logs_contains_exact("+CREG: 3"), message_logs_contains_exact("READY"));
    if (ready_message_index < 0) {
        state->initialized = false;
    } else {
        state->initialized = init_message_index < ready_message_index;
    }
    return state->initialized;
}

/**
 *
 * @param command
 * @return Returns true if command had to be sent, false if it was already sent
 */
bool send_command_if_not_already_sent(const char *command) {
    // Check if we need to send the command
    int command_sent_index = message_logs_contains_transmitted(command);

    if (command_sent_index < 0) {
        printf("Command %s not send, sending now\n", command);
        a9g_transmit(command, true);
        return true;
    }
    return false;
}

bool a9g_send_and_wait_for_command(const char *command) {
    if (send_command_if_not_already_sent(command)) return false;

    // Check if we got a response already
    int command_received_index = message_logs_contains_exact(command);

//    printf("%d / %d\t", command_received_index, message_log_length);
    if (command_received_index < 0 || message_log_length <= command_received_index + 1) {
//        printf("Command %s not received\n", command);
        return false;
    }

    bool command_is_ok = strcmp(message_log[command_received_index + 1], "OK") == 0;
    // Also check the next line for some cases (CGATT) returns OK after 1 line instead of after 0 lines
    if (message_log_length > command_received_index + 2) {
        command_is_ok |= strcmp(message_log[command_received_index + 2], "OK") == 0;
    }

    if (command_is_ok) {
        printf("Command %s is OK\n", command);
    } else {
//        printf("Command %s is not OK: %d '%s'\n", command, command_received_index,
//               message_log[command_received_index + 1]);
    }
    return command_is_ok;
}

bool a9g_check_if_we_have_network_connection() {
    if (send_command_if_not_already_sent("AT+CREG?")) return false;

    int command_received_index;
    for (command_received_index = message_log_length - 1; command_received_index >= 0; command_received_index--) {
        if (starts_with(message_log[command_received_index], "+CREG: ") &&
            message_log[command_received_index][9] == '1') {
            break;
        }
    }

    if (command_received_index < 0) return false;

    char *message = message_log[command_received_index];
    char status_code_char = message[strlen(message) - 1];
    int status_code = (int) status_code_char - 48;

    return status_code != 3 && status_code > -1;
}

enum SimStatus a9g_check_if_has_sim() {
    if (send_command_if_not_already_sent("AT+CPIN?")) return SIM_UNKNOWN;

    int command_received_index = message_logs_contains("+CPIN:", false);
    if (command_received_index < 0) return SIM_UNKNOWN;

    char *message = malloc(strlen(message_log[command_received_index]) + 1);
    char *original_message = message; // Store the original pointer
    strcpy(message, message_log[command_received_index]);
    extract_string(&message, NULL, ":");

    bool has_sim = strcmp(message, "READY") == 0;
    free(original_message); // Free the original pointer

    return has_sim ? SIM_PRESENT : SIM_NOT_PRESENT;
}

bool a9g_check_if_network_attached(A9GState *state) {
    if (state->network_attached) return true;

    if (!a9g_check_if_we_have_network_connection()) return false;
    state->network_attached = a9g_send_and_wait_for_command(A9G_CGATT_ENABLE);
    return state->network_attached;
}

bool a9g_check_if_pnp_parameters_set(A9GState *state) {
    if (state->pnp_parameters_set) return true;
    state->pnp_parameters_set = a9g_send_and_wait_for_command(A9G_CGDCONT_ENABLE);
    return state->pnp_parameters_set;
}

bool a9g_check_if_pnp_activated(A9GState *state) {
    if (state->pnp_activated) return true;
    state->pnp_activated = a9g_send_and_wait_for_command(A9G_CGACT_PNP_ENABLE);
    return state->pnp_activated;
}

bool a9g_check_if_agps_enabled(A9GState *state) {
    if (state->agps_enabled) return true;
    if (send_command_if_not_already_sent(A9G_AGPS_ENABLE)) return false;

    // Check if we got a response already
    int command_received_index = message_logs_contains_exact(A9G_AGPS_ENABLE);
    if (command_received_index < 0 || message_log_length <= command_received_index + 1) return false;

    bool command_is_ok = strcmp(message_log[command_received_index + 1], "+AGPS:GPD OK") == 0;
    if (message_log_length > command_received_index + 2) {
        command_is_ok |= strcmp(message_log[command_received_index + 2], "+AGPS:GPD OK") == 0;
    }

    if (command_is_ok) {
        state->agps_enabled = true;
        return true;
    }

    bool command_is_error = starts_with(message_log[command_received_index + 1], "+CME ERROR");
    if (message_log_length > command_received_index + 2) {
        command_is_error |= starts_with(message_log[command_received_index + 2], "+CME ERROR");
    }
    state->agps_enabled = command_is_error;
    return command_is_error;
}

bool a9g_check_if_gps_enabled(A9GState *state) {
    if (state->gps_enabled) return true;
    state->gps_enabled = a9g_send_and_wait_for_command(A9G_GPS_ENABLE);
    return state->gps_enabled;
}

bool a9g_check_if_gps_logging_enabled(A9GState *state) {
    if (state->gps_logging_enabled) return true;
    state->gps_logging_enabled = a9g_send_and_wait_for_command(A9G_GPSRD_ENABLE);
    return state->gps_logging_enabled;
}

void a9g_proceed_device_init(State *state) {
    if (message_log_length == 0) return;

    if (!a9g_check_if_init_done(&(state->a9g))) {
        a9g_state_reset(&(state->a9g));
        return;
    }

    if (state->gsm.sim_status == SIM_UNKNOWN) {
        state->gsm.sim_status = a9g_check_if_has_sim();
    }

    if (state->gsm.sim_status == SIM_UNKNOWN) return;
    if (state->gsm.sim_status == SIM_NOT_PRESENT) {
        if (!a9g_check_if_gps_enabled(&(state->a9g))) return;
        if (!a9g_check_if_gps_logging_enabled(&(state->a9g))) return;
        return;
    }

    if (!a9g_check_if_network_attached(&(state->a9g))) return;
    if (!a9g_check_if_pnp_parameters_set(&(state->a9g))) return;
    if (!a9g_check_if_pnp_activated(&(state->a9g))) return;
    if (!a9g_check_if_agps_enabled(&(state->a9g))) return;
    if (!a9g_check_if_gps_enabled(&(state->a9g))) return;
    if (!a9g_check_if_gps_logging_enabled(&(state->a9g))) return;
}

void a9g_process_messages(State *state) {
    if (message_log_length == 0) return;

    bool process_gngga_message_done = false;
    bool process_gnrmc_message_done = false;
    bool process_ctzv_message_done = false;

    for (int i = message_log_length - 1; i >= 0; i--) {
        if (!process_gngga_message_done &&
            (starts_with(message_log[i], "$GNGGA")
                || starts_with(message_log[i], "+GPSRD:$GNGGA"))) {
            state->a9g.gps_logging_started = true;
            state->location.is_gps_on = true;
            process_gngga_message(state, message_log[i]);
            process_gngga_message_done = true;
            continue;
        }
        if (!process_gnrmc_message_done && starts_with(message_log[i], "$GNRMC")) {
            state->a9g.gps_logging_started = true;
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

void a9g_validate_location_data(State *state) {
    // Reset location data after it becomes invalid (expires)
    if (esp_timer_get_time_ms() > state->location.gngga_last_updated + GPSGSM_LOCATION_MAX_VALID_TIME_MS) {
        state->location.altitude = 0;
        state->location.quality = 0;
        state->location.satellites = 0;

        state->location.time.minutes = 0;
        state->location.time.seconds = 0;
        state->location.time.hours = 0;
    }

    if (esp_timer_get_time_ms() > state->location.gngga_last_updated + GPSGSM_LOCATION_MAX_VALID_TIME_MS
        && esp_timer_get_time_ms() > state->location.gnrmc_last_updated + GPSGSM_LOCATION_MAX_VALID_TIME_MS) {
        state->location.latitude = 0;
        state->location.longitude = 0;
    }

    if (esp_timer_get_time_ms() > state->location.gnrmc_last_updated + GPSGSM_LOCATION_MAX_VALID_TIME_MS) {
        state->location.is_effective_positioning = false;
        state->location.ground_speed = 0;
        state->location.ground_heading = 0;
    }
}

void a9g_check_messages_timeout(State *state) {
    // Get last received message index
    int last_message_index = message_log_length - 1;
    while (last_message_index >= 0) {
        if (message_log[last_message_index][0] != '>') break;
        last_message_index--;
    }

    int64_t last_message_timestamp = 0;
    if (last_message_index >= 0) {
        last_message_timestamp = message_log_timestamps[last_message_index];

        if (esp_timer_get_time_ms() < last_message_timestamp + GPSGSM_MESSAGE_MAX_TIMEOUT_MS) {
            reset_error(state, ERROR_GPS_TIMEOUT);
            return;
        }

        printf("Last message has timed out: [%d] %lld >= %lld + %d: %s\n",
               last_message_index,
               esp_timer_get_time_ms(),
               last_message_timestamp,
               GPSGSM_MESSAGE_MAX_TIMEOUT_MS,
               message_log[last_message_index]);
    }

    // Check if reset was send
    int last_reset_message_index = message_logs_contains_transmitted(A9G_RESET);
    int64_t last_reset_message_timestamp = 0;
    if (last_reset_message_index >= 0) {
        last_reset_message_timestamp = message_log_timestamps[last_reset_message_index];

        if (esp_timer_get_time_ms() < last_reset_message_timestamp + GPSGSM_MESSAGE_MAX_TIMEOUT_MS) {
            reset_error(state, ERROR_GPS_TIMEOUT);
            return;
        }

        printf("Last reset message has timed out: [%d] %lld >= %lld + %d: %s\n",
               last_reset_message_index,
               esp_timer_get_time_ms(),
               last_reset_message_timestamp,
               GPSGSM_MESSAGE_MAX_TIMEOUT_MS,
               message_log[last_reset_message_index]);
    }

    if (message_log_length == 0 && esp_timer_get_time_ms() < GPSGSM_INIT_MAX_TIMEOUT_MS) {
        return;
    }
    if (message_log_length == 0) {
        printf("Last reset message has timed out: [%d] %lld no messages\n",
               message_log_length,
               esp_timer_get_time_ms());
    } else {
        printf("Last reset message has timed out: [%d] %lld unknown reason %d %d\n",
               message_log_length,
               esp_timer_get_time_ms(),
               last_message_index,
               last_reset_message_index);
    }

    debug_print_message_log();
    set_error(state, ERROR_GPS_TIMEOUT);
    a9g_reset(state);
}

void a9g_process(State *state) {
    a9g_receive(&state->a9g);

    update_time(state);
    a9g_validate_location_data(state);
    a9g_check_messages_timeout(state);

    a9g_proceed_device_init(state);
    a9g_process_messages(state);
}

void a9g_init(State *state) {
    gpsgsm_init(&state->a9g);
    a9g_reset(state);
}
