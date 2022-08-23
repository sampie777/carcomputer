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

void transmit(const char *data) {
    printf("[GPS] Sending\n");
    uart_write_bytes_with_break(GPSGSM_UART_NUMBER, data, strlen(data), 100);
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

    // Read data from UART.
    char data[128];
    int length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(GPSGSM_UART_NUMBER, (size_t *) &length));
    if (length == 0) {
        return;
    }
    length = uart_read_bytes(GPSGSM_UART_NUMBER, data, length, 100);
    data[min(length, 127)] = '\0';
    char *stripped_data = string_remove_chars(data, '\r');
    stripped_data = string_remove_chars(stripped_data, '\n');
    printf("[GPS] Received: '%s' with length: %d\n", stripped_data, length);
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
