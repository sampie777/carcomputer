//
// Created by samuel on 2024/09/27.
//

#include "led.h"
#include "../config.h"
#include "../utils.h"

#include <driver/gpio.h>

void led_init() {
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    led_set_off();
}

void led_set(bool value) {
    gpio_set_level(LED_PIN, value ? 1 : 0);
}

void led_blink(unsigned long interval) {
    static int64_t last_time = 0;
    static bool last_value = false;

    led_set(last_value);

    if (esp_timer_get_time_ms() < last_time + interval) return;
    last_time = esp_timer_get_time_ms();

    last_value = !last_value;
}