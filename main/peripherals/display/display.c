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
#include "../../version.h"
#include "../../error_codes.h"
#include "display_screens.h"

SH1106Config sh1106_config = {
        .address = DISPLAY_I2C_ADDRESS,
        .flip = DISPLAY_UPSIDE_DOWN,
        .width = DISPLAY_WIDTH,
        .height = DISPLAY_HEIGHT,
};

void display_init() {
    printf("[Display] Initializing display...\n");
    sh1106_init(&sh1106_config);
    printf("[Display] Init done\n");
}

void show_error_message(State *state, SH1106Config *sh1106) {
    static uint32_t current_error_to_show = 0;
    static int64_t last_error_message_time = 0;

    // If no new errors, return
    if (state->errors == 0) return;

    // After a timeout to show the previous error, determine the next error to show
    if (last_error_message_time == 0 || esp_timer_get_time_ms() > last_error_message_time + DISPLAY_ERROR_MESSAGE_TIME_MS) {
        // Get first error
        uint32_t new_error = 0;
        for (int i = 0; i < sizeof(state->errors) * 8; i++) {
            if (state->errors >> i == 1) {
                new_error = 1 << i;
                break;
            }
        }

        // Reset error
        state->errors &= ~new_error;

        if (new_error == 0 || new_error == current_error_to_show) {
            last_error_message_time = 0;
            return;
        }
        last_error_message_time = esp_timer_get_time_ms();
        current_error_to_show = new_error;
    }

    char buffer[32];
    switch (current_error_to_show) {
        case ERROR_PEDAL_DISCONNECTED:
            strcpy(buffer, "Pedal disconnected");
            break;
        case ERROR_SPI_FAILED:
            strcpy(buffer, "SPI failed");
            break;
        case ERROR_CRASH_NO_ICE:
            strcpy(buffer, "CRASH, no ICE!");
            break;
        case ERROR_CRASH_DETECTED:
            strcpy(buffer, "CRASH detected!");
            break;
        case ERROR_GPS_TIMEOUT:
            strcpy(buffer, "GPS timeout");
            break;
        case ERROR_SMS_FAILED:
            strcpy(buffer, "SMS failed");
            break;
        case ERROR_SD_FULL:
            strcpy(buffer, "SD car full");
            break;
        default:
            sprintf(buffer, "Code: %u", state->errors);
    }

    sh1106_draw_filled_rectangle(sh1106, 5, 5, sh1106->width - 10, sh1106->height - 10);
    sh1106_draw_string(sh1106, sh1106->width / 2 - 5 * 2, 7, FONT_SMALL, FONT_BLACK, "ERROR");
    sh1106_draw_string(sh1106, 10, 18, FONT_SMALL, FONT_BLACK, buffer);
}

void show_statusbar(State *state, SH1106Config *sh1106) {
    static int64_t last_long_blink_time = 0;
    static uint8_t long_blink_state = false;

    if (esp_timer_get_time_ms() > last_long_blink_time + DISPLAY_LONG_BLINK_INTERVAL) {
        long_blink_state = !long_blink_state;
        last_long_blink_time = esp_timer_get_time_ms();
    }

    if (state->cruise_control.enabled) {
        sh1106_draw_string(sh1106, 1, 1, FONT_SMALL, FONT_WHITE, "Cruise control");
    } else {
        sh1106_draw_string(sh1106, 1, 1, FONT_SMALL, FONT_WHITE, state->device_name);
    }

    int offset_right = sh1106->width + 1;

    offset_right -= 3 + icon_sd_width;
    if (state->storage.is_connected) {
        sh1106_draw_icon(sh1106, offset_right, 1,
                         icon_sd, sizeof(icon_sd), icon_sd_width, FONT_WHITE);
    }

    offset_right -= 3 + icon_car_width;
    if (state->car.is_connected || (long_blink_state && state->car.is_controller_connected)) {
        sh1106_draw_icon(sh1106, offset_right, 1,
                         icon_car, sizeof(icon_car), icon_car_width, FONT_WHITE);
    }

    offset_right -= 3 + icon_location_width;
    if (state->location.quality > 0 || (long_blink_state && state->location.is_gps_on)) {
        sh1106_draw_icon(sh1106, offset_right, 1,
                         icon_location, sizeof(icon_location), icon_location_width, FONT_WHITE);
    }

#if WIFI_ENABLE
    offset_right -= 3 + icon_wifi_width;
    if (state->wifi.is_connected || (long_blink_state && (state->wifi.is_scanning || state->wifi.is_connecting))) {
        sh1106_draw_icon(sh1106, offset_right, 1,
                         icon_wifi, sizeof(icon_wifi), icon_wifi_width, FONT_WHITE);
    }
#endif

    offset_right -= 3 + icon_data_width;
    if (state->server_is_uploading || state->gsm.is_uploading) {
        sh1106_draw_icon(sh1106, offset_right, 1,
                         icon_data, sizeof(icon_data), icon_data_width, FONT_WHITE);
    }

#if BLUETOOTH_ENABLE
    offset_right -= 3 + icon_bluetooth_width;
    if (state->bluetooth.connected) {
        sh1106_draw_icon(sh1106, offset_right, 0,
                         icon_bluetooth, sizeof(icon_bluetooth), icon_bluetooth_width, FONT_WHITE);
    }
#endif

    sh1106_draw_horizontal_line(sh1106, 0, STATUS_BAR_HEIGHT, sh1106->width);
}

void show_content_overlay(State *state, SH1106Config *sh1106) {
    if (state->power_off_count_down_sec > -1 && state->power_off_count_down_sec <= 10) {
        content_power_off_count_down(state, sh1106);
        return;
    }

    show_error_message(state, sh1106);
}

void show_screen(State *state, SH1106Config *sh1106) {
    switch (state->display.current_screen) {
        case Screen_Booting:
            sh1106_draw_string(sh1106, (sh1106->width - 5 * 10) / 2, STATUS_BAR_HEIGHT + (sh1106->height - STATUS_BAR_HEIGHT - 8) / 2 - 4,
                               FONT_SMALL, FONT_WHITE, "Booting...");
            sh1106_draw_string(sh1106, (sh1106->width - 5 * (int) strlen(APP_VERSION)) / 2, STATUS_BAR_HEIGHT + (sh1106->height - STATUS_BAR_HEIGHT - 8) / 2 + 7,
                               FONT_SMALL, FONT_WHITE, APP_VERSION);
            break;
        case Screen_Rebooting:
            sh1106_draw_string(sh1106, (sh1106->width - 5 * 12) / 2, STATUS_BAR_HEIGHT + (sh1106->height - STATUS_BAR_HEIGHT - 8) / 2,
                               FONT_SMALL, FONT_WHITE, "Rebooting...");
            break;
        case Screen_Registration:
            content_server_registration(state, sh1106);
            break;
        case Screen_Menu:
            content_main_menu(state, sh1106);
            break;
        case Screen_CruiseControl:
            content_cruise_control(state, sh1106);
            break;
        case Screen_Sensors:
            content_motion_sensors_data(state, sh1106);
            break;
        case Screen_GPS:
            content_location_data(state, sh1106);
            break;
        case Screen_Config:
            content_config(state, sh1106);
            break;
    }
}

void set_current_screen(State *state) {
    if (state->is_rebooting) {
        state->display.current_screen = Screen_Rebooting;
        return;
    }
    if (state->is_booting) {
        state->display.current_screen = Screen_Booting;
        return;
    }

    if (state->server.should_authenticate && state->server.registration_token != NULL && strlen(state->server.registration_token) > 0) {
        state->display.current_screen = Screen_Registration;
        return;
    }

    if (state->display.current_screen == Screen_Rebooting ||
        state->display.current_screen == Screen_Booting ||
        state->display.current_screen == Screen_Registration) {
        state->display.current_screen = Screen_Menu;
    }
}

void display_update(State *state) {
    static int64_t last_update_time = 0;
    if (esp_timer_get_time_ms() < last_update_time + DISPLAY_UPDATE_MIN_INTERVAL) return;
    last_update_time = esp_timer_get_time_ms();

    set_current_screen(state);

    sh1106_clear(&sh1106_config);

    show_statusbar(state, &sh1106_config);
    show_screen(state, &sh1106_config);
    show_content_overlay(state, &sh1106_config);

    sh1106_display(&sh1106_config);
}