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

#define MESSAGE_MAX_LENGTH 128

void transmit(const char *data) {
    printf("[GPS] Sending\n");
    uart_write_bytes_with_break(GPSGSM_UART_NUMBER, data, strlen(data), 100);
}

void process_message(const char *message) {
    if (strlen(message) == 0) {
        return;
    }
    printf("[GPS] Processing: '%s' with length: %d\n", message, strlen(message));
}

void read_messages() {
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
            process_message(last_message);
            last_message_index = 0;
            continue;
        }

        if (data[i] == '\n') continue;
        if (data[i] == '\r') {
            last_message[last_message_index] = '\0';
            process_message(last_message);
            last_message_index = 0;
            continue;
        }

        last_message[last_message_index++] = data[i];
    }
}

void gpsgsm_process() {
    static uint8_t gps_enabled = false;
    static uint8_t gps_logging_enabled = false;

    if (esp_timer_get_time_ms() > 10000 && !gps_enabled) {
        transmit("AT+GPS=1\r\n");
        gps_enabled = true;
    }

    if (esp_timer_get_time_ms() > 12000 && !gps_logging_enabled) {
        transmit("AT+GPSRD=1\r\n");
        gps_logging_enabled = true;
    }

    read_messages();
}

void gpsgsm_init() {
    printf("[GPS] Initializing...\n");
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
