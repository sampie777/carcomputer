//
// Created by samuel on 17-7-22.
//

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../config.h"
#include "display.h"
#include "../utils.h"
#include "sh1106.h"

SH1106Config sh1106 = {
        .address = DISPLAY_I2C_ADDRESS,
        .flip = DISPLAY_UPSIDE_DOWN,
};

void display_init() {
    printf("[Display] Initializing display...\n");
    sh1106_init(&sh1106);
    printf("[Display] Init done\n");
}

void show_error_message(State *state) {
    printf("[Display] Error message: %s\n", state->display.error_message);
//    ssd1306_display_text(&sh1106, 0, "ERROR", 5, true);
//    ssd1306_display_text(&sh1106, 1, state->display.error_message, (int) strlen(state->display.error_message), false);
    state->display.is_dirty = true;
}

void display_update(State *state) {
    if ((esp_timer_get_time_ms() / 1000) % 4 == 0) {
        sh1106_clear(&sh1106);
        return;
    }

    sh1106_display(&sh1106);
    return;
    printf("[Display] dirty [%s] isBooting [%s] esp_timer_get_time_ms(): %lu last_error_message_time: %lu\n",
           state->display.is_dirty ? "x" : " ",
           state->is_booting ? "x" : " ",
           esp_timer_get_time_ms(),
           state->display.last_error_message_time);

    if (!state->display.is_dirty) {
        return;
    }
    state->display.is_dirty = false;

//    ssd1306_clear_screen(&sh1106, false);

    if (esp_timer_get_time_ms() < state->display.last_error_message_time + DISPLAY_ERROR_MESSAGE_TIME_MS) {
        return show_error_message(state);
    }

    if (state->is_booting) {
//        ssd1306_display_text(&sh1106, 0, "Booting...", 10, false);
        return;
    }

//    ssd1306_display_text(&sh1106, 0, "Nissan Micra", 12, true);
//    ssd1306_display_text(&sh1106, 1, "Ready.", 5, false);

    printf("[Display] Speed: %3.1f\tWiFi: [%s] Bluetooth: [%s] CAN: [%s]\n",
           state->car.speed,
           state->wifi.connected ? "x" : " ",
           state->bluetooth.connected ? "x" : " ",
           state->car.connected ? "x" : " ");
}

void display_set_error_message(State *state, char *message) {
    printf("[Display] Set error message: %s\n", message);
    strncpy(state->display.error_message, message, DISPLAY_ERROR_MESSAGE_MAX_LENGTH);
    state->display.error_message[DISPLAY_ERROR_MESSAGE_MAX_LENGTH] = '\0';
    state->display.last_error_message_time = esp_timer_get_time_ms();
}