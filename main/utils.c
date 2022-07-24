//
// Created by samuel on 17-7-22.
//

#include <esp_timer.h>
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