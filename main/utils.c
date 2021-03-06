//
// Created by samuel on 17-7-22.
//

#include <esp_timer.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "utils.h"
#include "nvs_flash.h"

void nvs_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
}

unsigned long esp_timer_get_time_ms() {
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

int get_length(const char *s) {
    // Overflow will eventually make i < 0
    for (int16_t i = 0; i >= 0; i++) {
        if (s[i] == '\0') {
            return i;
        }
    }
    return -1;
}
