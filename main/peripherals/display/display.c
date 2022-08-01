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

#define STATUS_BAR_HEIGHT 9

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
    sh1106_draw_filled_rectangle(&sh1106, 5, 5, sh1106.width - 10, sh1106.height - 10);
    sh1106_draw_string(&sh1106, sh1106.width / 2 - 5 * 2, 7, FONT_SMALL, FONT_BLACK, 5, "ERROR");
    sh1106_draw_string(&sh1106, 10, 18, FONT_SMALL, FONT_BLACK, (int) strlen(state->display.error_message), state->display.error_message);
}

void show_statusbar(State *state) {
    if (state->cruise_control.enabled) {
        sh1106_draw_string(&sh1106, 1, 1, FONT_SMALL, FONT_WHITE, 14, "Cruise control");
    } else {
        sh1106_draw_string(&sh1106, 1, 1, FONT_SMALL, FONT_WHITE, sizeof(DEVICE_NAME), DEVICE_NAME);
    }

    sh1106_draw_horizontal_line(&sh1106, 0, STATUS_BAR_HEIGHT, sh1106.width);

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

void content_cruise_control(State *state) {
    int offset_x = 15;
    char buffer[20];
    int length = sprintf(buffer, "%3.0f / ", state->car.speed);
    offset_x += sh1106_draw_string(&sh1106, offset_x, STATUS_BAR_HEIGHT + 6, FONT_SMALL, FONT_WHITE, length, buffer);

    length = sprintf(buffer, "%.0f km/h", state->cruise_control.target_speed);
    sh1106_draw_string(&sh1106, offset_x, STATUS_BAR_HEIGHT + 6, FONT_SMALL, FONT_WHITE, length, buffer);

    // Animate virtual pedal position
    int virtual_pedal_container_y = STATUS_BAR_HEIGHT + 6;
    int virtual_pedal_container_height = sh1106.height - virtual_pedal_container_y - 10;
    int virtual_pedal_value_height = (int) (state->cruise_control.virtual_gas_pedal * virtual_pedal_container_height);
    int virtual_pedal_value_y = virtual_pedal_container_height - virtual_pedal_value_height;
    sh1106_draw_vertical_line(&sh1106, sh1106.width - 10, virtual_pedal_container_y, virtual_pedal_container_height);
    sh1106_draw_vertical_line(&sh1106, sh1106.width - 6, virtual_pedal_container_y, virtual_pedal_container_height);
    sh1106_draw_filled_rectangle(&sh1106, sh1106.width - 9, virtual_pedal_value_y, 3, virtual_pedal_value_height);
}

void motion_sensors_data(const State *state) {
    int offset_x = 0;
    int offset_y = STATUS_BAR_HEIGHT + 5;
    char buffer[20];

    offset_y += 10;
    sh1106_draw_string(&sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, 1, "x");
    offset_y += 10;
    sh1106_draw_string(&sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, 1, "y");
    offset_y += 10;
    sh1106_draw_string(&sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, 1, "z");
    offset_x += 9;
    offset_y = STATUS_BAR_HEIGHT + 5;

    sh1106_draw_string(&sh1106, offset_x + 1 * 5, offset_y, FONT_SMALL, FONT_WHITE, 5, "Accel");
    offset_y += 10;
    int length = sprintf(buffer, " %7.3f", state->motion.accel_x);
    sh1106_draw_string(&sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, length, buffer);
    offset_y += 10;
    length = sprintf(buffer, " %7.3f", state->motion.accel_y);
    sh1106_draw_string(&sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, length, buffer);
    offset_y += 10;
    length = sprintf(buffer, " %7.3f", state->motion.accel_z);
    sh1106_draw_string(&sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, length, buffer);
    offset_x += 7 * 5;
    offset_y += 10;

    length = sprintf(buffer, "Temp: %6.3f", state->motion.temperature);
    sh1106_draw_string(&sh1106, 0, offset_y, FONT_SMALL, FONT_WHITE, length, buffer);
    offset_y = STATUS_BAR_HEIGHT + 5;

    sh1106_draw_string(&sh1106, offset_x + 2 * 5, offset_y, FONT_SMALL, FONT_WHITE, 4, "Gyro");
    offset_y += 10;
    length = sprintf(buffer, " %7.2f", state->motion.gyro_x);
    sh1106_draw_string(&sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, length, buffer);
    offset_y += 10;
    length = sprintf(buffer, " %7.2f", state->motion.gyro_y);
    sh1106_draw_string(&sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, length, buffer);
    offset_y += 10;
    length = sprintf(buffer, " %7.2f", state->motion.gyro_z);
    sh1106_draw_string(&sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, length, buffer);
    offset_x += 7 * 5;
    offset_y = STATUS_BAR_HEIGHT + 5;

    sh1106_draw_string(&sh1106, offset_x + 2 * 5, offset_y, FONT_SMALL, FONT_WHITE, 4, "Comp");
    offset_y += 10;
    length = sprintf(buffer, " %7.1f", state->motion.compass_x);
    sh1106_draw_string(&sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, length, buffer);
    offset_y += 10;
    length = sprintf(buffer, " %7.1f", state->motion.compass_y);
    sh1106_draw_string(&sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, length, buffer);
    offset_y += 10;
    length = sprintf(buffer, " %7.1f", state->motion.compass_z);
    sh1106_draw_string(&sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, length, buffer);
}

void show_content(State *state) {
    if (state->is_rebooting) {
        sh1106_draw_string(&sh1106, (sh1106.width - 5 * 12) / 2, STATUS_BAR_HEIGHT + (sh1106.height - STATUS_BAR_HEIGHT - 8) / 2,
                           FONT_SMALL, FONT_WHITE, 12, "Rebooting...");
        return;
    }
    if (state->is_booting) {
        sh1106_draw_string(&sh1106, (sh1106.width - 5 * 10) / 2, STATUS_BAR_HEIGHT + (sh1106.height - STATUS_BAR_HEIGHT - 8) / 2,
                           FONT_SMALL, FONT_WHITE, 10, "Booting...");
        return;
    }

    if (state->display.last_error_message_time != 0 && esp_timer_get_time_ms() < state->display.last_error_message_time + DISPLAY_ERROR_MESSAGE_TIME_MS) {
        show_error_message(state);
        return;
    }

    if (state->cruise_control.enabled) {
        content_cruise_control(state);
        return;
    }

    motion_sensors_data(state);
}

void display_update(State *state) {
    static unsigned long last_update_time = 0;
    if (esp_timer_get_time_ms() < last_update_time + DISPLAY_UPDATE_MIN_INTERVAL) {
        return;
    }
    last_update_time = esp_timer_get_time_ms();

    sh1106_clear(&sh1106);

    show_statusbar(state);
    show_content(state);

    sh1106_display(&sh1106);
}

void display_set_error_message(State *state, char *message) {
    int is_new_message = false;
    for (int i = 0; i < DISPLAY_ERROR_MESSAGE_MAX_LENGTH; i++) {
        if (message[i] != state->display.error_message[i]) {
            is_new_message = true;
            break;
        }

        if (message[i] == '\0') {
            break;
        }
    }
    if (!is_new_message) return;

    printf("[Display] Set error message: %s\n", message);
    strncpy(state->display.error_message, message, DISPLAY_ERROR_MESSAGE_MAX_LENGTH);
    state->display.error_message[DISPLAY_ERROR_MESSAGE_MAX_LENGTH] = '\0';
    state->display.last_error_message_time = esp_timer_get_time_ms();
}