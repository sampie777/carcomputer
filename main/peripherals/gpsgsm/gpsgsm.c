//
// Created by samuel on 23-8-22.
//

#include <hal/uart_types.h>
#include <esp_check.h>
#include <driver/uart.h>
#include <hal/gpio_types.h>
#include <string.h>
#include "gpsgsm.h"
#include "../../utils.h"
#include "../display/display.h"
#include "utils.h"

#define MESSAGE_MAX_LENGTH 128

static enum SmsState sms_state = Idle;

static enum A9GCommand last_command_send = A9GCommand_Skip;

static char *post_url;
static char *post_body;

void process_gngga_message(State *state, const char *message) {
    NmeaGNGGAMessage gga_message = {0};
    extract_GNGGA_message(message, &gga_message);
    if (nmea_calculate_checksum(message) != gga_message.checksum) {
        // Checksum failed
        return;
    }

    state->location.latitude = nmea_coordinates_to_degrees(gga_message.latitude, gga_message.latitude_direction);
    state->location.longitude = nmea_coordinates_to_degrees(gga_message.longitude, gga_message.longitude_direction);
    state->location.altitude = gga_message.altitude;
    state->location.quality = gga_message.quality;
    state->location.satellites = gga_message.satellites;

    int hours = (int) (gga_message.timestamp / 10000);
    state->location.time.minutes = (int) (gga_message.timestamp / 100) - hours * 100;
    state->location.time.seconds = (int) gga_message.timestamp - hours * 10000 - state->location.time.minutes * 100;
    // Adjust for timezone
    hours += state->location.time.timezone;
    if (hours > 23) {
        hours -= 24;
    } else if (hours < 0) {
        hours += 24;
    }
    state->location.time.hours = hours;
}

void process_gnrmc_message(State *state, const char *message) {
    NmeaGNRMCMessage rmc_message = {0};
    extract_GNRMC_message(message, &rmc_message);
    if (nmea_calculate_checksum(message) != rmc_message.checksum) {
        // Checksum failed
        return;
    }

    state->location.is_effective_positioning = rmc_message.status == 'A';
    state->location.ground_speed = rmc_message.ground_speed * 1.852;    // knots -> km/h
    state->location.ground_heading = rmc_message.ground_heading;

    uint8_t day = rmc_message.date / 10000;
    uint8_t month = rmc_message.date / 100 - day * 100;
    uint16_t year = 2000 + rmc_message.date - day * 10000 - month * 100;

    if (year > 2021 && year < 2079) {
        state->location.time.day = day;
        state->location.time.month = month;
        state->location.time.year = year;
    }
}

void transmit(const char *data, uint8_t with_break) {
    if (with_break) {
        uart_write_bytes_with_break(GPSGSM_UART_NUMBER, data, strlen(data), 50);
    } else {
        uart_write_bytes(GPSGSM_UART_NUMBER, data, strlen(data));
    }
}

/**
 * Send the data string in smaller pieces at a time to reduce riskt for lengthy transfers
 * @param data
 * @param max_transfer_size
 */
void transmit_safe(const char *data, size_t max_transfer_size, uint8_t with_break) {
    size_t length = strlen(data);
    for (size_t i = 0; i < length; i += max_transfer_size) {
        if (with_break && i + max_transfer_size > length) {
            uart_write_bytes_with_break(GPSGSM_UART_NUMBER, &data[i], min(max_transfer_size, length - i), 50);
        } else {
            uart_write_bytes(GPSGSM_UART_NUMBER, &data[i], min(max_transfer_size, length - i));
        }
    }
}

void send_command(A9GState *a9g_state, enum A9GCommand command) {
    last_command_send = command;
    switch (command) {
        case A9GCommand_CGATT_Enable:
            printf("[GPS] Attach to network\n");
            transmit(A9G_CGATT_ENABLE, true);
            a9g_state->network_attached = A9Status_Requested;
            break;
        case A9GCommand_CGACT_PNP_Enable:
            printf("[GPS] Activate PNP\n");
            transmit(A9G_CGACT_PNP_ENABLE, true);
            a9g_state->pnp_activated = A9Status_Requested;
            break;
        case A9GCommand_CGDCONT_Enable:
            printf("[GPS] Set PNP parameters\n");
            transmit(A9G_CGDCONT_ENABLE, true);
            a9g_state->pnp_parameters_set = A9Status_Requested;
            break;
        case A9GCommand_AGPS_Disable:
            printf("[GPS] Disable AGPS\n");
            transmit(A9G_AGPS_DISABLE, true);
            a9g_state->agps_enabled = A9Status_Requested;
            break;
        case A9GCommand_AGPS_Enable:
            printf("[GPS] Enable AGPS\n");
            transmit(A9G_AGPS_ENABLE, true);
            a9g_state->agps_enabled = A9Status_Requested;
            break;
        case A9GCommand_GPSRD_Enable:
            printf("[GPS] Enable GPS logging\n");
            transmit(A9G_GPSRD_ENABLE, true);
            a9g_state->gps_logging_enabled = A9Status_Requested;
            break;
        default:
            break;
    }
}

void process_message(State *state, const char *message) {
    if (strlen(message) == 0) {
        return;
    }

    if (!starts_with(message, "$")) {
        printf("[GPS] Processing: '%s' with length: %d\n", message, strlen(message));
    }

    if (strcmp(message, "Init...") == 0) {
        a9g_state_reset(state->a9g);
        state->a9g->initialized = A9Status_Requested;
    } else if (starts_with(message, "READY") || starts_with(message, "Ai_Thinker_Co")) {
        a9g_state_reset(state->a9g);
        state->a9g->initialized = A9Status_Ok;
    } else if (starts_with(message, "+CMGS=")) {
        sms_state = SentSuccess;
    }

    if (strcmp(message, "OK") == 0 || strstr(message, " OK") != NULL) {
        switch (last_command_send) {
            case A9GCommand_CGATT_Enable:
                state->a9g->network_attached = A9Status_Ok;
                break;
            case A9GCommand_CGACT_PNP_Enable:
                state->a9g->pnp_activated = A9Status_Ok;
                break;
            case A9GCommand_CGDCONT_Enable:
                state->a9g->pnp_parameters_set = A9Status_Ok;
                break;
            case A9GCommand_AGPS_Disable:
                state->a9g->agps_enabled = A9Status_Disabled;
                break;
            case A9GCommand_AGPS_Enable:
                state->a9g->agps_enabled = A9Status_Ok;
                break;
            case A9GCommand_GPSRD_Enable:
                state->a9g->gps_logging_enabled = A9Status_Ok;
                break;
            default:
                break;
        }
    }

    if (strstr(message, "$GNGGA") != NULL) {
        state->a9g->gps_logging_started = true;
        state->a9g->gps_logging_enabled = A9Status_Ok;
        process_gngga_message(state, message);
    } else if (strstr(message, "$GNRMC") != NULL) {
        process_gnrmc_message(state, message);
    }
}

void read_messages(State *state) {
    static char *last_message;
    static int last_message_index = 0;

    if (last_message == NULL) {
        last_message = malloc(MESSAGE_MAX_LENGTH);
    }

    // Read data from UART.
    int length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(GPSGSM_UART_NUMBER, (size_t *) &length));
    if (length == 0) {
        return;
    }

    char data[MESSAGE_MAX_LENGTH];
    length = uart_read_bytes(GPSGSM_UART_NUMBER, data, min(length, MESSAGE_MAX_LENGTH), 100);

    for (int i = 0; i < length; i++) {
        if (last_message_index >= MESSAGE_MAX_LENGTH) {
            printf("[GPS] Max message length reached\n");
            last_message[MESSAGE_MAX_LENGTH - 1] = '\0';
            process_message(state, last_message);
            last_message_index = 0;
            continue;
        }

        if (data[i] == '\n') continue;
        if (data[i] == '\r') {
            last_message[last_message_index] = '\0';
            process_message(state, last_message);
            last_message_index = 0;
            continue;
        }

        last_message[last_message_index++] = data[i];
    }
}

void proceed_device_init(State *state) {
    static int64_t state_start_time = 0;
    static A9GState previous_state = {};

    if (!a9g_state_compare(state->a9g, &previous_state)) {
        state_start_time = esp_timer_get_time_ms();
        a9g_state_clone(state->a9g, &previous_state);
    }

    if (state->a9g->initialized == A9Status_Unknown || state->a9g->initialized == A9Status_Requested) {
        if (esp_timer_get_time_ms() > state_start_time + GPSGSM_INIT_MAX_TIMEOUT_MS) {
            printf("[GPS] WARNING: Init timeout\n");
            state->a9g->initialized = A9Status_Ok;
        }
    } else if (state->a9g->network_attached != A9Status_Ok) {
        if (state->a9g->network_attached != A9Status_Requested) {
            send_command(state->a9g, A9GCommand_CGATT_Enable);
        }
    } else if (state->a9g->pnp_parameters_set != A9Status_Ok) {
        if (state->a9g->pnp_parameters_set != A9Status_Requested) {
            send_command(state->a9g, A9GCommand_CGDCONT_Enable);
        }
    } else if (state->a9g->pnp_activated != A9Status_Ok) {
        if (state->a9g->pnp_activated != A9Status_Requested) {
            send_command(state->a9g, A9GCommand_CGACT_PNP_Enable);
        }
    } else if (state->a9g->agps_enabled == A9Status_Unknown || state->a9g->agps_enabled == A9Status_Error) {
        if (state->a9g->agps_enabled != A9Status_Requested) {
            send_command(state->a9g, A9GCommand_AGPS_Disable);
        }
    } else if (state->a9g->agps_enabled != A9Status_Ok) {
        if (state->a9g->agps_enabled != A9Status_Requested) {
            send_command(state->a9g, A9GCommand_AGPS_Enable);
        }
    } else if (state->a9g->gps_logging_enabled != A9Status_Ok) {
        if (state->a9g->gps_logging_enabled != A9Status_Requested) {
            send_command(state->a9g, A9GCommand_GPSRD_Enable);
        }
    } else if (!state->a9g->gps_logging_started && esp_timer_get_time_ms() > state_start_time + GPSGSM_MESSAGE_MAX_TIMEOUT_MS) {
        printf("[GPS] ERROR: Initial NMEA message timeout\n");
        display_set_error_message(state, "GPS timeout");

        // Retry GPS initiation
        a9g_state_reset(state->a9g);
        state->a9g->initialized = A9Status_Error;
    }
}

void gpsgsm_process(State *state) {
    static int64_t sms_sent_time = 0;

    proceed_device_init(state);

    if (sms_state == Sending) {
        if (sms_sent_time == 0) {
            sms_sent_time = esp_timer_get_time_ms();
        } else if (esp_timer_get_time_ms() > sms_sent_time + GPSGSM_SMS_SENT_MAX_TIMEOUT_MS) {
            sms_state = SentFailed;
        }
    } else if (sms_state == SentSuccess) {
        sms_state = Idle;
        sms_sent_time = 0;
    } else if (sms_state == SentFailed) {
        display_set_error_message(state, "SMS failed");
        sms_state = Idle;
        sms_sent_time = 0;
    }

    read_messages(state);

    state->location.is_gps_on = state->a9g->gps_logging_started;

    if (state->gsm.is_uploading && esp_timer_get_time_ms() > state->gsm.upload_start_time + 5000) {
        // Send data to the server
        transmit("AT+HTTPPOST=\"", false);
        transmit_safe(post_url, 8, false);
        transmit("\",\"application/json\",\"", false);
        transmit_safe(post_body, 8, false);
        transmit("\"\r", true);

        free(post_url);
        free(post_body);
        state->gsm.is_uploading = false;
    }
}

void gpsgsm_init(A9GState *a9g_state) {
    printf("[GPS] Initializing...\n");
    a9g_state->initialized = A9Status_Unknown;

    uart_config_t uart_config = {
            .baud_rate = GPSGSM_UART_BAUD_RATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 122,
    };
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(GPSGSM_UART_NUMBER, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GPSGSM_UART_NUMBER, GPSGSM_UART_TX_PIN, GPSGSM_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Setup UART buffered IO with event queue
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(GPSGSM_UART_NUMBER, uart_buffer_size, \
                                        uart_buffer_size, 10, &uart_queue, 0));

    a9g_state->initialized = A9Status_Requested;
    printf("[GPS] Init done\n");
}

void gsm_send_sms(const char *number, const char *message) {
    printf("[GSM] Sending SMS to %s with content: '%s'\n", number, message);

    char buffer[32];
    // Enable text mode
    transmit("AT+CMGF=1\r", true);
    delay_ms(100);

    // Start SMS to number
    sprintf(buffer, "AT+CMGS=%s\r", number);
    transmit(buffer, true);
    delay_ms(500);

    // Insert SMS message
    transmit_safe(message, 8, false);
    transmit("\r", true);
    delay_ms(500);

    // Send SMS
    sprintf(buffer, "%c\r", 0x1a);
    transmit(buffer, true);
    sms_state = Sending;
}

void gsm_http_post(State *state, const char *url, const char *json) {
    printf("[GSM] Sending data to %s\n", url);
    state->gsm.is_uploading = true;
    state->gsm.upload_start_time = esp_timer_get_time_ms();

    // Strip domain from url
    char url_copy[strlen(url)];
    strcpy(url_copy, url);
    strtok(url_copy, "://");
    char *domain = strtok(NULL, "/");

    // Store url for upload
    if (post_url) free(post_url);
    post_url = malloc(strlen(url) + 1);
    strcpy(post_url, url);
    post_url[strlen(url)] = '0';

    char *json_escaped;
    string_escape(json, &json_escaped);
    printf("[GSM] JSON %s\n", json_escaped);

    if (!json_escaped) {
        printf("[GSM] JSON string points to null\n");
        state->gsm.is_uploading = false;
        return;
    }

    // Store body for upload
    if (post_body) free(post_body);
    post_body = malloc(strlen(json_escaped) + 1);
    strcpy(post_body, json_escaped);
    post_body[strlen(json_escaped)] = '0';
    free(json_escaped);

    // Open connection to the server
    transmit("AT+CGATT=1\r", false);
    transmit_safe("AT+CIPSTART=\"TCP\",\"", 8, false);
    transmit_safe(domain, 8, false);
    transmit("\",80\r", true);

    // Wait in process() for connection to establish
}