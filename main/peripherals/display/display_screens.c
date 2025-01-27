//
// Created by samuel on 9-12-22.
//

#include <math.h>
#include "display_screens.h"
#include "display.h"
#include "../../utils.h"


void draw_check_box(SH1106Config* sh1106, int x, int y, int size, bool checked) {
    sh1106_draw_rectangle(sh1106, x, y, size, size);
    if (!checked) return;
    sh1106_draw_filled_rectangle(sh1106, x + 2, y + 2, max(0, size - 2 * 2), max(0, size - 2 * 2));
}

void content_main_menu_option(SH1106Config* sh1106, int y, int height, const char* text, bool highlighted) {
    if (highlighted) {
        sh1106_draw_filled_rectangle(sh1106, 0, y, sh1106->width, height);
    }
    sh1106_draw_string(sh1106, 5, y + (height - 8) / 2, FONT_SMALL,
                       highlighted ? FONT_BLACK : FONT_WHITE, text);
}

void content_cruise_control(State* state, SH1106Config* sh1106) {
    int offset_x = 5;
    int offset_y = STATUS_BAR_HEIGHT + 10;
    char buffer[20];
    sprintf(buffer, "%3.0f%s ", state->car.speed, state->cruise_control.enabled ? "/" : " km/h");
    offset_x += sh1106_draw_string(sh1106, offset_x, offset_y, FONT_MEDIUM, FONT_WHITE, buffer);

    if (state->cruise_control.enabled) {
        sprintf(buffer, "%.0f", state->cruise_control.target_speed);
        sh1106_draw_string(sh1106, offset_x, offset_y, FONT_LARGE, FONT_WHITE, buffer);
    }

    offset_y += 10 * FONT_LARGE;
    offset_x = 5;

    switch (state->car.estimated_gear) {
        case GearNeutral:
            sprintf(buffer, "Gear: N");
            break;
        case GearReverse:
            sprintf(buffer, "Gear: R");
            break;
        default:
            sprintf(buffer, "Gear: %d", state->car.estimated_gear);
    }
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);

    if (!state->cruise_control.enabled) return;

    // Animate virtual pedal position
    // Draw the container
    int virtual_pedal_container_y = STATUS_BAR_HEIGHT + 6;
    int virtual_pedal_container_height = sh1106->height - virtual_pedal_container_y - 10;
    sh1106_draw_vertical_line(sh1106, sh1106->width - 5, virtual_pedal_container_y, virtual_pedal_container_height);
    sh1106_draw_vertical_line(sh1106, sh1106->width - 1, virtual_pedal_container_y, virtual_pedal_container_height);
    sh1106_draw_horizontal_line(sh1106, sh1106->width - 4, virtual_pedal_container_y - 1, 3);
    sh1106_draw_horizontal_line(sh1106, sh1106->width - 4, virtual_pedal_container_y + virtual_pedal_container_height,
                                3);

    // Draw the value
    int virtual_pedal_value_height = (int) (state->cruise_control.virtual_gas_pedal * virtual_pedal_container_height);
    int virtual_pedal_value_y = virtual_pedal_container_y + virtual_pedal_container_height - virtual_pedal_value_height;
    sh1106_draw_filled_rectangle(sh1106, sh1106->width - 4 + 1, virtual_pedal_value_y, 2, virtual_pedal_value_height);
}

void content_power_off_count_down(State* state, SH1106Config* sh1106) {
    int length, offset_x, offset_y;
    char buffer[20];
    int margin = 5;
    int row_height = 8 + 2 * margin;

    length = sprintf(buffer, "Powering off in...");
    offset_x = (sh1106->width - length * 5) / 2;
    offset_y = STATUS_BAR_HEIGHT + (sh1106->height - STATUS_BAR_HEIGHT - 2 * row_height) / 2;
    sh1106_draw_filled_rectangle(sh1106, offset_x - margin, offset_y,
                                 sh1106->width - 2 * offset_x + 2 * margin, row_height);
    sh1106_draw_string(sh1106, offset_x, offset_y + margin,
                       FONT_SMALL, FONT_BLACK, buffer);

    sprintf(buffer, "%d", state->power_off_count_down_sec);
    offset_x = (sh1106->width - length * 5) / 2;
    offset_y += row_height;
    sh1106_draw_filled_rectangle(sh1106, offset_x - margin, offset_y,
                                 sh1106->width - 2 * offset_x + 2 * margin, row_height);
    sh1106_draw_string(sh1106, offset_x, offset_y + margin,
                       FONT_SMALL, FONT_BLACK, buffer);
}

void content_motion_sensors_data(const State* state, SH1106Config* sh1106) {
    int offset_x = 0;
    int offset_y = STATUS_BAR_HEIGHT + 5;
    char buffer[20];

    offset_y += 10;
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, "x");
    offset_y += 10;
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, "y");
    offset_y += 10;
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, "z");
    offset_x += 9;
    offset_y = STATUS_BAR_HEIGHT + 5;

    sh1106_draw_string(sh1106, offset_x + 1 * 5, offset_y, FONT_SMALL, FONT_WHITE, " Accel");
    offset_y += 10;
    sprintf(buffer, "%7.2f", state->motion.accel_x);
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, "%7.2f", state->motion.accel_y);
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, "%7.2f", state->motion.accel_z);
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_x += 7 * 5 + 1;
    offset_y += 12;

    sprintf(buffer, "G: %6.2f",
            sqrt(state->motion.accel_x * state->motion.accel_x + state->motion.accel_y * state->motion.accel_y + state->
                motion.accel_z * state->motion.accel_z));
    sh1106_draw_string(sh1106, 0, offset_y, FONT_SMALL, FONT_WHITE, buffer);

    sprintf(buffer, "  Temp:  %5.1f", state->motion.temperature);
    sh1106_draw_string(sh1106, 11 * 5, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y = STATUS_BAR_HEIGHT + 5;

    sh1106_draw_string(sh1106, offset_x + 2 * 5, offset_y, FONT_SMALL, FONT_WHITE, " Gyro");
    offset_y += 10;
    sprintf(buffer, " %7.1f", state->motion.gyro_x);
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, " %7.1f", state->motion.gyro_y);
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, " %7.1f", state->motion.gyro_z);
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_x += 7 * 5 + 1;
    offset_y = STATUS_BAR_HEIGHT + 5;

    sh1106_draw_string(sh1106, offset_x + 2 * 5, offset_y, FONT_SMALL, FONT_WHITE, " Comp");
    offset_y += 10;
    sprintf(buffer, " %7.0f", state->motion.compass_x);
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, " %7.0f", state->motion.compass_y);
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, " %7.0f", state->motion.compass_z);
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
}

char* content_actions_get_option_text(ActionsScreenOptions option_index) {
    switch (option_index) {
        case ScreenActionsOptions_LockDoors:
            return "Lock doors";
        case ScreenActionsOptions_ErrorCodes:
            return "Read error codes";
        case ScreenActionsOptions_Reboot:
            return "Reboot";
        default:
            return "";
    }
}

void content_actions(const State* state, SH1106Config* sh1106) {
    static int options_start_index = 0;
    const int selection_item_height = 12;
    const int options_total_height = sh1106->height - STATUS_BAR_HEIGHT - 2;
    const int total_displayable_options = options_total_height / selection_item_height;
    char* buffer = NULL;

    // Move window so it fits the selected option
    if (state->display.actions_option_selection >= options_start_index + total_displayable_options) {
        // If selection is beyond <start index> + <total amount of displayable options>, increase the <start index>
        options_start_index = (int) state->display.actions_option_selection - total_displayable_options + 1;
    } else if (state->display.actions_option_selection < options_start_index) {
        // If selection is less than the <start index>, decrease the <start index>
        options_start_index = state->display.actions_option_selection;
    }

    // Iterate up to options size, in order to force text overflow at the bottom of the display
    for (int i = 0; i <= total_displayable_options; i++) {
        int y = STATUS_BAR_HEIGHT + 2 + i * selection_item_height;
        buffer = content_actions_get_option_text(options_start_index + i);
        content_main_menu_option(sh1106, y, selection_item_height, buffer,
                                 state->display.actions_option_selection == options_start_index + i);
    }
}

void content_location_data(const State* state, SH1106Config* sh1106) {
    int offset_x = 0;
    int offset_y = STATUS_BAR_HEIGHT + 5;
    char buffer[32];

    if (!state->a9g.gps_logging_started) {
        if (state->a9g.gps_logging_enabled) {
            sprintf(buffer, "Waiting for GPS logs...");
        } else if (state->a9g.agps_enabled) {
            sprintf(buffer, "Enabling GPS logging...");
        } else if (state->a9g.pnp_activated) {
            sprintf(buffer, "Enabling GPS...");
        } else if (state->a9g.pnp_parameters_set) {
            sprintf(buffer, "Activating PNP...");
        } else if (state->a9g.network_attached) {
            sprintf(buffer, "Setting PNP parameters...");
        } else if (state->a9g.initialized) {
            sprintf(buffer, "Attaching to network...");
        } else {
            sprintf(buffer, "GPS module booting...");
        }
        sh1106_draw_string(sh1106, 1, offset_y + 5, FONT_SMALL, FONT_WHITE, buffer);
        return;
    }

    sprintf(buffer, "%d:%02d:%02d    %d-%02d-%04d",
            state->location.time.hours,
            state->location.time.minutes,
            state->location.time.seconds,
            state->location.time.day,
            state->location.time.month,
            state->location.time.year
    );
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;

    sprintf(buffer, "%.5lf, %.5lf", state->location.latitude, state->location.longitude);
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, "Q:%d S:%d E:%d A:%.0lf",
            state->location.quality,
            state->location.satellites,
            state->location.is_effective_positioning,
            state->location.altitude);
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, "%6.2lf km/h @ %6.1lf*", state->location.ground_speed, state->location.ground_heading);
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
}

void content_error_codes(const State* state, SH1106Config* display) {
    int offset_x = 0;
    int offset_y = STATUS_BAR_HEIGHT + 5 + 10;
    char buffer[32] = {0};

    switch (state->error_codes.status) {
        case ErrorCodes_IgnitionOff:
            sh1106_draw_string_centered_horizontally(display, offset_y, FONT_SMALL, FONT_WHITE, "Turn ignition off.");
            return;
        case ErrorCodes_IgnitionOn:
            sh1106_draw_string_centered_horizontally(display, offset_y, FONT_SMALL, FONT_WHITE, "Turn ignition on.");
            offset_y += 15;
            sh1106_draw_string_centered_horizontally(display, offset_y, FONT_SMALL, FONT_WHITE,
                                                     "Do NOT start the car!");
            return;
        case ErrorCodes_ReleasePedal:
            sh1106_draw_string_centered_horizontally(display, offset_y, FONT_SMALL, FONT_WHITE, "Done.");
            return;
        default:
            break;
    }

    double total_time = (double) (state->error_codes.process_estimated_end_time -
        state->error_codes.process_start_time);
    double current_time = (double) (esp_timer_get_time_ms() - state->error_codes.process_start_time);
    double progress = min(1.0, max(0, current_time / total_time));
    double remaining_time = (double) max(0, (state->error_codes.process_estimated_end_time - esp_timer_get_time_ms()));

    sh1106_draw_string_centered_horizontally(display, offset_y, FONT_SMALL, FONT_WHITE, "Running sequence...");
    offset_y += 10;

    // Animate progress
    // Draw the container
    int virtual_pedal_container_x = 20;
    int virtual_pedal_container_width = display->width - virtual_pedal_container_x * 2;
    sh1106_draw_horizontal_line(display, virtual_pedal_container_x, offset_y, virtual_pedal_container_width);
    sh1106_draw_horizontal_line(display, virtual_pedal_container_x, offset_y + 4, virtual_pedal_container_width);
    sh1106_draw_vertical_line(display, virtual_pedal_container_x - 1, offset_y, 3);
    sh1106_draw_vertical_line(display, virtual_pedal_container_x + virtual_pedal_container_width, offset_y, 3);

    // Draw the value
    int virtual_pedal_value_width = (int) (progress * virtual_pedal_container_width);
    int virtual_pedal_value_x = virtual_pedal_container_x + virtual_pedal_container_width - virtual_pedal_value_width;
    sh1106_draw_filled_rectangle(display, virtual_pedal_value_x, offset_y + 2, virtual_pedal_value_width, 2);

    offset_y += 10;

    snprintf(buffer, sizeof buffer, "%.0f seconds remaining...", ceil(remaining_time / 1000));
    sh1106_draw_string_centered_horizontally(display, offset_y, FONT_SMALL, FONT_WHITE, buffer);

    offset_y += 15;
    sh1106_draw_string_centered_horizontally(display, offset_y, FONT_SMALL, FONT_WHITE, "Do NOT start the car!");
}

char* content_main_menu_get_option_text(MainMenuScreenOptions option_index) {
    switch (option_index) {
        case ScreenMenuOption_CruiseControl:
            return "Cruise control";
        case ScreenMenuOption_Sensors:
            return "Sensors";
        case ScreenMenuOption_Actions:
            return "Actions";
        case ScreenMenuOption_GPS:
            return "GPS";
        default:
            return "";
    }
}

void content_main_menu(const State* state, SH1106Config* sh1106) {
    static int options_start_index = 0;
    const int selection_item_height = 12;
    const int options_total_height = sh1106->height - STATUS_BAR_HEIGHT - 2;
    const int total_displayable_options = options_total_height / selection_item_height;
    char* buffer = NULL;

    // Move window so it fits the selected option
    if (state->display.menu_option_selection >= options_start_index + total_displayable_options) {
        // If selection is beyond <start index> + <total amount of displayable options>, increase the <start index>
        options_start_index = (int) state->display.menu_option_selection - total_displayable_options + 1;
    } else if (state->display.menu_option_selection < options_start_index) {
        // If selection is less than the <start index>, decrease the <start index>
        options_start_index = state->display.menu_option_selection;
    }

    // Iterate up to options size, in order to force text overflow at the bottom of the display
    for (int i = 0; i <= total_displayable_options; i++) {
        int y = STATUS_BAR_HEIGHT + 2 + i * selection_item_height;
        buffer = content_main_menu_get_option_text(options_start_index + i);
        content_main_menu_option(sh1106, y, selection_item_height, buffer,
                                 state->display.menu_option_selection == options_start_index + i);
    }
}
