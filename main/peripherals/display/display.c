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
#include "../../return_codes.h"
#include "special_chars.h"
#include "font.h"
#include "sh1106_i2c.h"

SH1106Config sh1106_config = {
    .address = DISPLAY_I2C_ADDRESS,
    .mirror_vertical = DISPLAY_UPSIDE_DOWN,
    .width = DISPLAY_WIDTH,
    .height = DISPLAY_HEIGHT,
};

void display_init() {
    printf("[Display] Initializing display...\n");
    if (sh1106_init(&sh1106_config) != RESULT_OK) {
        printf("[Display] Init failed\n");
        return;
    }
    printf("[Display] Init done\n");
}

void show_error_message(State *state, SH1106Config *display) {
    static uint32_t current_error_to_show = 0;
    static int64_t last_error_message_time = 0;

    // If no new errors, return
    if (state->errors == 0) return;

    // After a timeout to show the previous error, determine the next error to show
    if (last_error_message_time == 0
        || esp_timer_get_time_ms() > last_error_message_time + DISPLAY_ERROR_MESSAGE_TIME_MS) {
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
        case ERROR_CAR_DISCONNECTED:
            strcpy(buffer, "Car disconnected");
            break;
        default:
            sprintf(buffer, "Code: %lu", state->errors);
    }

    sh1106_draw_filled_rectangle(display, 5, 5, display->width - 10, display->height - 10);
    sh1106_draw_string_centered_x(display, 7, FONT_SMALL, FONT_BLACK, "ERROR");
    sh1106_draw_string(display, 10, 18, FONT_SMALL, FONT_BLACK, buffer);
}

void show_statusbar(State *state, SH1106Config *display) {
    static int64_t last_long_blink_time = 0;
    static int64_t last_short_blink_time = 0;
    static uint8_t long_blink_state = false;
    static uint8_t short_blink_state = false;
    char buffer[4];

    if (esp_timer_get_time_ms() > last_long_blink_time + DISPLAY_LONG_BLINK_INTERVAL) {
        last_long_blink_time = esp_timer_get_time_ms();
        long_blink_state = !long_blink_state;

        last_short_blink_time = last_long_blink_time;
        short_blink_state = long_blink_state;
    }

    if (esp_timer_get_time_ms() > last_short_blink_time + DISPLAY_SHORT_BLINK_PULSE_LENGTH) {
        last_short_blink_time = esp_timer_get_time_ms();
        short_blink_state = false;
    }

    if (state->cruise_control.enabled) {
        sh1106_draw_string(display, 1, 0, FONT_SMALL, FONT_WHITE, "Cruise control");
    } else {
        sh1106_draw_string(display, 1, 0, FONT_SMALL, FONT_WHITE, APP_VERSION);
    }

    int offset_right = display->width + 1;

    offset_right -= 3 + font_width;
    if (state->storage.is_connected && (state->storage.filename[0] != 0 || long_blink_state)) {
        sprintf(buffer, "%c", SPECIAL_CHAR_SD_CARD);
        sh1106_draw_string(display, offset_right, 1, FONT_SMALL, FONT_WHITE, buffer);
    }

    offset_right -= 3 + icon_car_width;
    if (state->car.is_connected || (long_blink_state && state->car.is_controller_connected)) {
        sh1106_draw_icon(display, offset_right, 1,
                         icon_car, sizeof(icon_car), icon_car_width, FONT_WHITE);
    }

    offset_right -= 3 + font_width;
    if (state->location.quality > 0
        || (long_blink_state && state->location.is_gps_on)
        || (short_blink_state && state->a9g.initialized)
        ) {
        sprintf(buffer, "%c", SPECIAL_CHAR_LOCATION);
        sh1106_draw_string(display, offset_right, 1, FONT_SMALL, FONT_WHITE, buffer);
    }

    sh1106_draw_horizontal_line(display, 0, STATUS_BAR_HEIGHT, display->width);
}

void show_content_overlay(State *state, SH1106Config *display) {
    if (state->power_off_count_down_sec > -1 && state->power_off_count_down_sec <= 10) {
        content_power_off_count_down(state, display);
        return;
    }

    show_error_message(state, display);
}

void show_screen(State *state, SH1106Config *display) {
    switch (state->display.current_screen) {
        case Screen_Booting:
            content_boot_screen(state, display);
            break;
        case Screen_Rebooting:
            sh1106_draw_string_centered_x(display, STATUS_BAR_HEIGHT + (display->height - STATUS_BAR_HEIGHT - 8) / 2,
                                          FONT_SMALL, FONT_WHITE, "Rebooting...");
            break;
        case Screen_Menu:
            content_main_menu(state, display);
            break;
        case Screen_CruiseControl:
            content_cruise_control(state, display);
            break;
        case Screen_Sensors: {
            switch (state->display.subscreen.sensors) {
                case ScreenSensors_SensorsMotionValues:
                    content_motion_sensors_data(state, display);
                    break;
                case ScreenSensors_SensorsMotionGraphical:
                    content_motion_sensors_data_graphical(state, display);
                    break;
                case ScreenSensors_SensorsInputs:
                    content_sensors_input(state, display);
                    break;
                case ScreenSensors_GPS:
                    content_location_data(state, display);
                    break;
                default:
                    break;
            }
            break;
        }
        case Screen_Actions:
            content_actions(state, display);
            break;
        case Screen_ActivateDiagnostics:
            content_activate_diagnostics(state, display);
            break;
        case Screen_About: {
            switch (state->display.subscreen.sensors) {
                case ScreenAbout_SD:
                    content_about(state, display);
                    break;
                case ScreenAbout_AboutCruiseControl:
                    content_about_cruise_control(state, display);
                    break;
                case ScreenAbout_AboutCar:
                    content_about_car(state, display);
                    break;
                default:
                    break;
            }
            break;
        }
    }
}

void set_current_screen(State *state) {
    if (state->boot.is_rebooting) {
        state->display.current_screen = Screen_Rebooting;
        return;
    }
    if (state->boot.is_booting) {
        state->display.current_screen = Screen_Booting;
        return;
    }

    if (state->display.current_screen == Screen_Rebooting ||
        state->display.current_screen == Screen_Booting) {
        state->display.current_screen = Screen_CruiseControl;
    }
}

void check_if_was_disconnected(SH1106Config* display) {
    if (display->transmission_failures == 0) return;

    // This means the display was disconnected at some point, so we need to re-init it
    display_init();

    display->transmission_failures = 0;
}

void display_update(State *state) {
    static int64_t last_update_time = 0;
    if (esp_timer_get_time_ms() < last_update_time + DISPLAY_UPDATE_MIN_INTERVAL) return;
    last_update_time = esp_timer_get_time_ms();

    check_if_was_disconnected(&sh1106_config);

    set_current_screen(state);

    sh1106_clear(&sh1106_config);

    show_statusbar(state, &sh1106_config);
    show_screen(state, &sh1106_config);
    show_content_overlay(state, &sh1106_config);

    sh1106_display(&sh1106_config);
}
