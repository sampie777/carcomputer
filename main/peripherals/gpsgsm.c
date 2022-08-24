//
// Created by samuel on 23-8-22.
//

#include <hal/uart_types.h>
#include <esp_check.h>
#include <driver/uart.h>
#include <hal/gpio_types.h>
#include <string.h>
#include "gpsgsm.h"
#include "../config.h"
#include "../utils.h"
#include "display/display.h"
/*

  MCU                  A9G
  # Check connection
  AT -->
                   <-- OK

  # Enable GPS
  AT+GPS=1 -->
                   <-- OK
  # Check GPS status
  AT+GPS? -->
                   <-- +GPS: 1

  AT+GPSRD=1 -->
                   <-- OK
  AT+GPSRD? -->
                   <-- +GPSRD:1

 */

#define MESSAGE_MAX_LENGTH 128


enum ConnectionState {
    ColdBoot = 0,
    Initializing,
    Initialized,
    GpsEnableRequestSent,
    GpsEnableRequestApproved,
    GpsEnableNmeaLoggingRequestSent,
    GpsEnableNmeaLoggingRequestApproved,
};
static enum ConnectionState connection_state = Initializing;

void transmit(const char *data) {
    printf("[GPS] Sending\n");
    uart_write_bytes_with_break(GPSGSM_UART_NUMBER, data, strlen(data), 50);
}

void enable_gps() {
    delay_ms(10);
    transmit("AT+GPS=1\r\n");
    connection_state = GpsEnableRequestSent;
}

void enable_gps_logging() {
    delay_ms(10);
    transmit("AT+GPSRD=1\r\n");
    connection_state = GpsEnableNmeaLoggingRequestSent;
}

void process_message(State *state, const char *message) {
    if (strlen(message) == 0) {
        return;
    }
    printf("[GPS] Processing: '%s' with length: %d\n", message, strlen(message));

    if (strcmp(message, "Init...") == 0) {
        connection_state = Initializing;
    } else if (strcmp(message, "+CIEV") == 0) {
        connection_state = Initialized;
    } else if (connection_state == GpsEnableRequestSent && strcmp(message, "OK") == 0) {
        connection_state = GpsEnableRequestApproved;
    } else if (connection_state == GpsEnableNmeaLoggingRequestSent && strcmp(message, "OK") == 0) {
        connection_state = GpsEnableNmeaLoggingRequestApproved;
        state->location.is_connected = true;
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

void gpsgsm_process(State *state) {
    static int64_t state_start_time = 0;
    static enum ConnectionState previous_state = 0xff;

    if (connection_state != previous_state) {
        state_start_time = esp_timer_get_time_ms();
        previous_state = connection_state;
    }

    if (connection_state != GpsEnableNmeaLoggingRequestApproved && esp_timer_get_time_ms() > state_start_time + GPSGSM_INIT_MAX_TIMEOUT_MS) {
        printf("[GPS] Init timeout reached for state: %d. Going to previous state.\n", connection_state);
        display_set_error_message(state, "GPS failed");

        // Go to previous state
        if (connection_state > 0) {
            connection_state--;
        }
    }

    if (connection_state == ColdBoot) {
        connection_state = Initializing;
    } else if (connection_state == Initialized) {
        enable_gps();
    } else if (connection_state == GpsEnableRequestApproved) {
        enable_gps_logging();
    }

    read_messages(state);

    state->location.is_connected = connection_state == GpsEnableNmeaLoggingRequestApproved;
}

void gpsgsm_init() {
    printf("[GPS] Initializing...\n");
    connection_state = ColdBoot;

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

    printf("[GPS] Init done\n");
}
