//
// Created by Samuel-Anton Jansen on 2025/11/13.
//

#include "uart.h"


esp_err_t uart_param_config(uart_port_t uart_num, const uart_config_t *uart_config) {
    return ESP_OK;
}

esp_err_t uart_set_pin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num) {
    return ESP_OK;
}

esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int event_queue_size,
                              QueueHandle_t *uart_queue, int intr_alloc_flags) {
    return ESP_OK;
}
