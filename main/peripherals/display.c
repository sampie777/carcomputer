//
// Created by samuel on 17-7-22.
//

#include <stdio.h>
#include <freertos/portmacro.h>
#include <string.h>
#include "../config.h"
#include "display.h"
#include "../lib/esp-idf-ssd1306/components/ssd1306/ssd1306.h"

SSD1306_t sh1106 = {
        ._address = DISPLAY_I2C_ADDRESS,
        ._flip = DISPLAY_UPSIDE_DOWN,
};

void display_init() {
    printf("[Display] Initializing display...\n");
    ssd1306_i2c_init(&sh1106, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    ssd1306_clear_screen(&sh1106, false);
    ssd1306_contrast(&sh1106, 0xff);
    ssd1306_display_text_x3(&sh1106, 0, "booting...", 10, false);
    printf("[Display] Init done\n");
}

void show_error_message(State *state) {
    printf("[Display] Error message: %s\n", state->display.error_message);
}

void display_update(State *state) {
    if (esp_timer_get_time() < state->display.last_error_message_time + DISPLAY_ERROR_MESSAGE_TIME) {
        return show_error_message(state);
    }

    printf("[Display] Speed: %3.1f\tWiFi: [%s] Bluetooth: [%s] CAN: [%s]\n",
           state->car.speed,
           state->wifi.connected ? "x" : " ",
           state->bluetooth.connected ? "x" : " ",
           state->car.connected ? "x" : " ");
}

void display_set_error_message(State *state, char *message) {
    strncpy(state->display.error_message, message, DISPLAY_ERROR_MESSAGE_MAX_LENGTH);
    state->display.last_error_message_time = esp_timer_get_time();
}