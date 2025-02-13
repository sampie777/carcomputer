//
// Created by samuel on 23-8-22.
//

#include <hal/uart_types.h>
#include <esp_check.h>
#include <driver/uart.h>
#include <hal/gpio_types.h>
#include <string.h>
#include "gpsgsm.h"
#include "../../config.h"
#include "../../utils.h"
#include "utils.h"
#include "../../error_codes.h"

#define MESSAGE_MAX_LENGTH 512

//static enum SmsState sms_state = Idle;

//static char *http_request_url = NULL;
//static char *http_request_body = NULL;

QueueHandle_t uart_queue;

// static void (*http_request_callback)(State *state, const HttpResponseMessage *response) = NULL;

void process_gngga_message(State *state, const char *message) {
    NmeaGNGGAMessage decoded_message = {0};
    extract_GNGGA_message(message, &decoded_message);
    if (nmea_calculate_checksum(message) != decoded_message.checksum) {
        // Checksum failed
        return;
    }

    state->location.latitude = nmea_coordinates_to_degrees(decoded_message.latitude,
                                                           decoded_message.latitude_direction);
    state->location.longitude = nmea_coordinates_to_degrees(decoded_message.longitude,
                                                            decoded_message.longitude_direction);
    state->location.altitude = decoded_message.altitude;
    state->location.quality = decoded_message.quality;
    state->location.satellites = decoded_message.satellites;

    int hours = (int) (decoded_message.timestamp / 10000);
    state->location.time.minutes = (int) (decoded_message.timestamp / 100) - hours * 100;
    state->location.time.seconds = (int) decoded_message.timestamp - hours * 10000 - state->location.time.minutes * 100;
    state->location.time.hours = hours;

    state->location.gngga_last_updated = esp_timer_get_time_ms();
}

void process_gnrmc_message(State *state, const char *message) {
    NmeaGNRMCMessage decoded_message = {0};
    extract_GNRMC_message(message, &decoded_message);
    if (nmea_calculate_checksum(message) != decoded_message.checksum) {
        // Checksum failed
        return;
    }

    state->location.latitude = nmea_coordinates_to_degrees(decoded_message.latitude,
                                                           decoded_message.latitude_direction);
    state->location.longitude = nmea_coordinates_to_degrees(decoded_message.longitude,
                                                            decoded_message.longitude_direction);

    state->location.is_effective_positioning = decoded_message.status == 'A';
    state->location.ground_speed = decoded_message.ground_speed * 1.852;    // knots -> km/h
    state->location.ground_heading = decoded_message.ground_heading;

    uint8_t day = decoded_message.date / 10000;
    uint8_t month = decoded_message.date / 100 - day * 100;
    uint16_t year = 2000 + decoded_message.date - day * 10000 - month * 100;

    if (year > 2021 && year < 2079) {
        state->location.time.day = day;
        state->location.time.month = month;
        state->location.time.year = year;
    }

    state->location.gnrmc_last_updated = esp_timer_get_time_ms();
}

void process_ctzv_message(State *state, const char *message) {
    Time time = {0};
    extract_ctzv_message(message, &time);

    // We can ignore day etc, as we don't expect the device running that long
    uint64_t total_seconds = time.seconds
        + time.minutes * 60
        + time.hours * 3600;
    uint64_t total_seconds_current = state->gsm.time.seconds
        + state->gsm.time.minutes * 60
        + state->gsm.time.hours * 3600;
    if (total_seconds <= total_seconds_current) return;

    memcpy(&(state->gsm.time), &time, sizeof(time));

    printf("[GSM] Set new GSM time: %04d-%02d-%02d'T'%02d:%02d:%02d.000%+d\n",
           state->gsm.time.year,
           state->gsm.time.month,
           state->gsm.time.day,
           state->gsm.time.hours,
           state->gsm.time.minutes,
           state->gsm.time.seconds,
           state->gsm.time.timezone
    );
}

void send_command(A9GState *a9g_state, enum A9GCommand command) {
}

void process_http_response(State *state, const char *message, const char *stripped_message) {
    // static int http_code = -1;
    // static int content_length = -1;
    // static int messages_since_http_start = 0;
    // if (http_request_callback == NULL) return;
    //
    // if (http_code < 0 && starts_with(stripped_message, "HTTP/1.1  ")) {
    //     messages_since_http_start = 0;
    //     int result = sscanf(message, "HTTP/1.1  %d", &http_code);
    //     if (result == 0) {
    //         return;
    //     }
    // }
    //
    // if (http_code > 0) {
    //     if (messages_since_http_start++ > 30) {
    //         printf("[GSM] HTTP response message time out");
    //         http_code = -1;
    //         content_length = -1;
    //         return;
    //     }
    // }
    //
    // if (content_length < 0 && starts_with(stripped_message, "Content-Length: ")) {
    //     // Get length
    //     int result = sscanf(message, "Content-Length: %d", &content_length);
    //     printf("[GSM] HTTP Content length will be %d\n", content_length);
    //     if (result == 0) {
    //         return;
    //     }
    // }
    //
    // if (content_length <= 0) return;
    //
    // if (strlen(message) != content_length) return;
    //
    // printf("[GSM] Processing HTTP: '%s' with length: %d\n", message, strlen(message));
    //
    // // Filter out header fields
    // if (starts_with(stripped_message, "Date: ") ||
    //     starts_with(stripped_message, "Content-Type: ") ||
    //     starts_with(stripped_message, "Content-Length: ") ||
    //     starts_with(stripped_message, "Server: ") ||
    //     starts_with(stripped_message, "Access-Control-Allow-Origin: ") ||
    //     starts_with(stripped_message, "Access-Control-Allow-Credentials: ") ||
    //     starts_with(stripped_message, "X-Frame-Options: ") ||
    //     starts_with(stripped_message, "X-Content-Type-Options: ") ||
    //     starts_with(stripped_message, "X-XSS-Protection: ") ||
    //     starts_with(stripped_message, "Connection: "))
    //     return;
    //
    // HttpResponseMessage response = {
    //         .code = http_code,
    //         .content_length = content_length,
    //         .message = malloc(strlen(message) + 1),
    // };
    // strcpy(response.message, message);
    //
    // http_request_callback(state, &response);
    // http_request_callback = NULL;
    //
    // http_code = -1;
    // content_length = -1;
    // free(response.message);
}

void process_message(State *state, const char *message) {
    char *stripped_message = malloc(strlen(message) + 1);
    strcpy(stripped_message, message);
    string_char_remove(&stripped_message, '\n');

    if (strlen(stripped_message) == 0) {
        free(stripped_message);
        return;
    }

    process_http_response(state, message, stripped_message);

    if (!starts_with(stripped_message, "$")) {
        printf("[GPS] Processing: '%s' with length: %d\n", stripped_message, strlen(stripped_message));
    }

    if (starts_with(stripped_message, "+CMGS=")) {
//        sms_state = SentSuccess;
    }

    if (strstr(stripped_message, "$GNGGA") != NULL) {
    } else if (strstr(stripped_message, "$GNRMC") != NULL) {
    } else if (starts_with(stripped_message, "+CTZV:")) {
    } else if (strcmp(stripped_message, "failure, pelase check your network or certificate!") == 0) {
        state->a9g.network_error_count++;

        if (state->a9g.network_error_count > 1) {
            printf("[GSM] Restarting GSM network (error count: %u)\n", state->a9g.network_error_count);
            // Resetting the whole chip seems like the only solution
            // Related: https://stackoverflow.com/questions/68612918/https-requests-on-a9g-via-at-commands-fail-after-7-requests-http-works-fine
            send_command(&state->a9g, A9GCommand_Reset_Software);
        }
    }

    free(stripped_message);
}


void update_time(State *state) {
    static int64_t last_update_time = 0;

    if (esp_timer_get_time_ms() < last_update_time + 1000) return;
    last_update_time = esp_timer_get_time_ms();

    state->gsm.time.seconds++;

    if (state->gsm.time.seconds < 60) return;
    state->gsm.time.seconds = 0;
    state->gsm.time.minutes++;

    if (state->gsm.time.minutes < 60) return;
    state->gsm.time.minutes = 0;
    state->gsm.time.hours++;

    if (state->gsm.time.hours < 24) return;
    state->gsm.time.hours = 0;
    state->gsm.time.day++;

    if (state->gsm.time.day < 30) return;
    if ((state->gsm.time.month == 1 ||
        state->gsm.time.month == 3 ||
        state->gsm.time.month == 5 ||
        state->gsm.time.month == 7 ||
        state->gsm.time.month == 8 ||
        state->gsm.time.month == 10 ||
        state->gsm.time.month == 12) && state->gsm.time.day == 30
        )
        return;
    state->gsm.time.day = 1;
    state->gsm.time.month++;

    if (state->gsm.time.month < 13) return;
    state->gsm.time.month = 1;
    state->gsm.time.year++;
}

void gpsgsm_process(State *state) {
//    static int64_t sms_sent_time = 0;

    // if (sms_state == Sending) {
    //     if (sms_sent_time == 0) {
    //         sms_sent_time = esp_timer_get_time_ms();
    //     } else if (esp_timer_get_time_ms() > sms_sent_time + GPSGSM_SMS_SENT_MAX_TIMEOUT_MS) {
    //         sms_state = SentFailed;
    //     }
    // } else if (sms_state == SentSuccess) {
    //     sms_state = Idle;
    //     sms_sent_time = 0;
    // } else if (sms_state == SentFailed) {
    //     set_error(state, ERROR_SMS_FAILED);
    //     sms_state = Idle;
    //     sms_sent_time = 0;
    // }

    // if (state->gsm.is_uploading && esp_timer_get_time_ms() > state->gsm.upload_start_time + 5000) {
    //     // Send data to the server
    //     switch (state->gsm.request_type) {
    //         case HTTP_METHOD_GET:
    //             transmit("AT+HTTPGET=\"", false);
    //             transmit_safe(http_request_url, 8, false);
    //             transmit("\"\r", true);
    //             break;
    //         case HTTP_METHOD_POST: {
    //             transmit("AT+HTTPPOST=\"", false);
    //             transmit_safe(http_request_url, 8, false);
    //             transmit("\",\"application/json\",\"", false);
    //             transmit_safe(http_request_body, 8, false);
    //             transmit("\"\r", true);
    //             break;
    //         }
    //         default:
    //             printf("[GSM] Unhandled HTTP request type: %d\n", state->gsm.request_type);
    //     }
    //
    //     if (http_request_url != NULL) free(http_request_url);
    //     http_request_url = NULL;
    //     if (http_request_body != NULL) free(http_request_body);
    //     http_request_body = NULL;
    //     state->gsm.is_uploading = false;
    // }

}

void gpsgsm_init(A9GState *a9g_state) {
    printf("[GPS] Initializing...\n");
    a9g_state->initialized = false;

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
    ESP_ERROR_CHECK(uart_set_pin(GPSGSM_UART_NUMBER, GPSGSM_UART_TX_PIN, GPSGSM_UART_RX_PIN, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE));

    // Setup UART buffered IO with event queue
    // Install UART driver using an event queue here
    // Multiplying the BUF_SIZE by a factor of 2 in the uart_driver_install function call is done to allocate a larger buffer for both the transmit and receive buffers. This helps to ensure that there is enough space to handle the data being transmitted and received, reducing the risk of buffer overflows and data loss, especially when dealing with high data rates or large amounts of data.
    ESP_ERROR_CHECK(uart_driver_install(GPSGSM_UART_NUMBER, A9G_UART_BUFFER_SIZE * 2, \
                                        A9G_UART_BUFFER_SIZE * 2, 10, &uart_queue, 0));

    printf("[GPS] Init done\n");
}

// void gsm_send_sms(const char *number, const char *message) {
//     printf("[GSM] Sending SMS to %s with content: '%s'\n", number, message);
//
//     char buffer[32];
//     // Enable text mode
//     transmit("AT+CMGF=1\r", true);
//     delay_ms(100);
//
//     // Start SMS to number
//     snprintf(buffer, sizeof buffer, "AT+CMGS=%s\r", number);
//     transmit(buffer, true);
//     delay_ms(500);
//
//     // Insert SMS message
//     transmit_safe(message, 8, false);
//     transmit("\r", true);
//     delay_ms(500);
//
//     // Send SMS
//     sprintf(buffer, "%c\r", 0x1a);
//     transmit(buffer, true);
//     sms_state = Sending;
// }

// void gsm_http_get(State *state, const char *url, void (*callback)(State *state, const HttpResponseMessage *response)) {
//     printf("[GSM] HTTP GET request to %s\n", url);
//     state->gsm.is_uploading = true;
//     state->gsm.upload_start_time = esp_timer_get_time_ms();
//     state->gsm.request_type = HTTP_METHOD_GET;
//     http_request_callback = callback;
//
//     // Strip domain from url
//     char url_copy[strlen(url) + 1];
//     strcpy(url_copy, url);
//     strtok(url_copy, "://");
//     char *domain = strtok(NULL, "/");
//
//     // Store url for upload
//     http_request_url = realloc(http_request_url, strlen(url) + 1);
//     strcpy(http_request_url, url);
//
//     // Open connection to the server
//     printf("[GSM] Open connection\n");
//     transmit_safe("AT+CIPSTART=\"TCP\",\"", 8, false);
//     transmit_safe(domain, 8, false);
//     transmit("\",80\r", true);
//
//     // Wait in process() for connection to establish
//     printf("[GSM] Wait for response\n");
// }

// void gsm_http_post(State *state, const char *url, const char *json) {
//     printf("[GSM] HTTP POST request to %s\n", url);
//     state->gsm.is_uploading = true;
//     state->gsm.upload_start_time = esp_timer_get_time_ms();
//     state->gsm.request_type = HTTP_METHOD_POST;
//
//     // Strip domain from url
//     char url_copy[strlen(url)];
//     strcpy(url_copy, url);
//     strtok(url_copy, "://");
//     char *domain = strtok(NULL, "/");
//
//     // Store url for upload
//     http_request_url = realloc(http_request_url, strlen(url) + strlen(state->server.access_token) + strlen(SERVER_API_KEY) + 32);
//     snprintf(http_request_url, sizeof http_request_url, "%s%capi_key=%s&access_token=%s", url, strstr(url, "?") == NULL ? '?' : '&', SERVER_API_KEY, state->server.access_token);
//
//     char *json_escaped;
//     string_escape(json, &json_escaped);
//     printf("[GSM] JSON %s\n", json_escaped);
//
//     if (!json_escaped) {
//         printf("[GSM] JSON string points to null\n");
//         state->gsm.is_uploading = false;
//         return;
//     }
//
//     // Store body for upload
//     http_request_body = realloc(http_request_body, strlen(json_escaped) + 1);
//     strcpy(http_request_body, json_escaped);
//     free(json_escaped);
//
//     // Open connection to the server
//     transmit(A9G_CGATT_ENABLE, true);
//     transmit_safe("AT+CIPSTART=\"TCP\",\"", 8, false);
//     transmit_safe(domain, 8, false);
//     transmit("\",80\r", true);
//
//     // Wait in process() for connection to establish
// }