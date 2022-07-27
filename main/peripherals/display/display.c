//
// Created by samuel on 17-7-22.
//

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../../config.h"
#include "display.h"
#include "../../utils.h"
#include "sh1106.h"
#include "icons.h"

SH1106Config sh1106 = {
        .address = DISPLAY_I2C_ADDRESS,
        .flip = DISPLAY_UPSIDE_DOWN,
        .width = DISPLAY_WIDTH,
        .height = DISPLAY_HEIGHT,
};

void display_init() {
    printf("[Display] Initializing display...\n");
    sh1106_init(&sh1106);
    sh1106_clear(&sh1106);
    printf("[Display] Init done\n");
}

void show_error_message(State *state) {
    printf("[Display] Error message: %s\n", state->display.error_message);
    sh1106_draw_filled_rectangle(&sh1106, 5, 5, sh1106.width - 10, sh1106.height - 10);
    sh1106_draw_string(&sh1106, sh1106.width / 2 - 5 * 2, 7, FONT_SMALL, "ERROR", 5, FONT_BLACK);
    sh1106_draw_string(&sh1106, 10, 18, FONT_SMALL, state->display.error_message, (int) strlen(state->display.error_message), FONT_BLACK);
    state->display.is_dirty = true;
}

void show_statusbar(State *state) {
    sh1106_draw_string(&sh1106, 1, 1, FONT_SMALL, "Hello World", 11, FONT_WHITE);
    sh1106_draw_horizontal_line(&sh1106, 0, 9, sh1106.width);

    int offset_right = sh1106.width - 2 - icon_wifi_width;
    if (state->wifi.connected) {
        sh1106_draw_icon(&sh1106, offset_right, 1,
                         icon_wifi, sizeof(icon_wifi), icon_wifi_width, FONT_WHITE);
    }

    offset_right -= 3 + icon_bluetooth_width;
    if (state->bluetooth.connected) {
        sh1106_draw_icon(&sh1106, offset_right, 0,
                         icon_bluetooth, sizeof(icon_bluetooth), icon_bluetooth_width, FONT_WHITE);
    }

    offset_right -= 3 + icon_car_width;
    if (state->car.connected) {
        sh1106_draw_icon(&sh1106, offset_right, 1,
                         icon_car, sizeof(icon_car), icon_car_width, FONT_WHITE);
    }
}

void show_content(State *state) {
    if (state->display.last_error_message_time != 0 && esp_timer_get_time_ms() < state->display.last_error_message_time + DISPLAY_ERROR_MESSAGE_TIME_MS) {
        show_error_message(state);
    }
}

void display_update(State *state) {
    sh1106_clear(&sh1106);

    show_statusbar(state);
    show_content(state);

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
        show_error_message(state);
    } else if (state->is_booting) {
//        ssd1306_display_text(&sh1106, 0, "Booting...", 10, false);
    } else {
//    ssd1306_display_text(&sh1106, 0, "Nissan Micra", 12, true);
//    ssd1306_display_text(&sh1106, 1, "Ready.", 5, false);

        printf("[Display] Speed: %3.1f\tWiFi: [%s] Bluetooth: [%s] CAN: [%s]\n",
               state->car.speed,
               state->wifi.connected ? "x" : " ",
               state->bluetooth.connected ? "x" : " ",
               state->car.connected ? "x" : " ");
    }
    sh1106_display(&sh1106);
}

void display_set_error_message(State *state, char *message) {
    printf("[Display] Set error message: %s\n", message);
    strncpy(state->display.error_message, message, DISPLAY_ERROR_MESSAGE_MAX_LENGTH);
    state->display.error_message[DISPLAY_ERROR_MESSAGE_MAX_LENGTH] = '\0';
    state->display.last_error_message_time = esp_timer_get_time_ms();
}