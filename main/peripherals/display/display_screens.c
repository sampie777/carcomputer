//
// Created by samuel on 9-12-22.
//

#include <math.h>
#include "display_screens.h"
#include "display.h"
#include "../../utils.h"
#include "../adc.h"
#include "special_chars.h"
#include "../../version.h"
#include "font.h"

/**
 * Display all available ASCII characters on the current screen.
 * @param state
 * @param display
 */
void content_alphabet(const State *state, SH1106Config *display) {
    int offset_y = STATUS_BAR_HEIGHT + 5;
    char buffer[64];

    static int64_t last_time = 0;
    static uint8_t last_char = 0;

    uint8_t current_char = last_char;
    for (int y = 0; y < (display->height - STATUS_BAR_HEIGHT) / 10; y++) {
        for (int x = 0; x < display->width / (font_width + 1); x++) {
            if ((x > 0 || y > 0) && current_char == 0) break;

            sprintf(buffer, "%c", current_char++);
            sh1106_draw_string(display, x * (font_width + 1), offset_y + y * 10, FONT_SMALL, FONT_WHITE, buffer);
        }
    }

    if (esp_timer_get_time_ms() < last_time + 10000) return;
    last_time = esp_timer_get_time_ms();

    last_char = current_char;
}

void draw_check_box(SH1106Config *sh1106, int x, int y, int size, bool checked) {
    sh1106_draw_rectangle(sh1106, x, y, size, size);
    if (!checked) return;
    sh1106_draw_filled_rectangle(sh1106, x + 2, y + 2, max(0, size - 2 * 2), max(0, size - 2 * 2));
}

void content_main_menu_option(SH1106Config *sh1106, int y, int height, const char *text, bool highlighted) {
    if (highlighted) {
        sh1106_draw_filled_rectangle(sh1106, 0, y, sh1106->width, height);
    }
    sh1106_draw_string(sh1106, 5, y + (height - 8) / 2, FONT_SMALL,
                       highlighted ? FONT_BLACK : FONT_WHITE, text);
}

char *content_main_menu_get_option_text(MainMenuScreenOptions option_index) {
    switch (option_index) {
        case ScreenMenuOption_CruiseControl:
            return "Cruise control";
        case ScreenMenuOption_Sensors:
            return "Sensors";
        case ScreenMenuOption_Actions:
            return "Actions";
        case ScreenMenuOption_GPS:
            return "GPS";
        case ScreenMenuOption_About:
            return "About";
        default:
            return "";
    }
}

void content_main_menu(const State *state, SH1106Config *display) {
    static int options_start_index = 0;
    const int selection_item_height = 12;
    const int options_total_height = display->height - STATUS_BAR_HEIGHT - 2;
    const int total_displayable_options = options_total_height / selection_item_height;
    char *buffer = NULL;

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
        content_main_menu_option(display, y, selection_item_height, buffer,
                                 state->display.menu_option_selection == options_start_index + i);
    }
}

void content_cruise_control(State *state, SH1106Config *display) {
    int offset_x = 5;
    int offset_y = STATUS_BAR_HEIGHT + 10;
    char buffer[20];
    snprintf(buffer, sizeof buffer, "%3.0f%s ", state->car.speed, state->cruise_control.enabled ? "/" : " km/h");
    offset_x += sh1106_draw_string(display, offset_x, offset_y, FONT_MEDIUM, FONT_WHITE, buffer);

    if (state->cruise_control.enabled) {
        sprintf(buffer, "%.0f", state->cruise_control.target_speed);
        sh1106_draw_string(display, offset_x, offset_y, FONT_LARGE, FONT_WHITE, buffer);
    }

    offset_y += 8 * FONT_LARGE + 2;
    offset_x = 5;

    switch (state->car.estimated_gear) {
        case GearNeutral:
            sprintf(buffer, "N");
            break;
        case GearReverse:
            sprintf(buffer, "R");
            break;
        default:
            snprintf(buffer, sizeof buffer, "%d", state->car.estimated_gear);
    }
    sh1106_draw_string(display, offset_x, offset_y, FONT_MEDIUM, FONT_WHITE, buffer);

    // --- Debug stuff

    offset_x += 20;
    offset_y += 7;
    sprintf(buffer, "%.1f m/s%c", state->car.acceleration, SPECIAL_CHAR_POWER2);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);

    offset_y -= 9;
    double ratio = state->car.rpm == 0 ? 0 : state->car.speed / state->car.rpm_raw * 10000;
    sprintf(buffer, "%.1f", ratio);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);

    // --- End of debug stuff

    if (!state->cruise_control.enabled) return;

    // Animate virtual pedal position
    // Draw the container
    int virtual_pedal_container_y = STATUS_BAR_HEIGHT + 6;
    int virtual_pedal_container_height = display->height - virtual_pedal_container_y - 10;
    sh1106_draw_vertical_line(display, display->width - 5, virtual_pedal_container_y, virtual_pedal_container_height);
    sh1106_draw_vertical_line(display, display->width - 1, virtual_pedal_container_y, virtual_pedal_container_height);
    sh1106_draw_horizontal_line(display, display->width - 4, virtual_pedal_container_y - 1, 3);
    sh1106_draw_horizontal_line(display, display->width - 4, virtual_pedal_container_y + virtual_pedal_container_height,
                                3);

    // Draw the value
    int virtual_pedal_value_height = (int) (state->cruise_control.virtual_gas_pedal * virtual_pedal_container_height);
    int virtual_pedal_value_y = virtual_pedal_container_y + virtual_pedal_container_height - virtual_pedal_value_height;
    sh1106_draw_filled_rectangle(display, display->width - 4 + 1, virtual_pedal_value_y, 2, virtual_pedal_value_height);
}

void content_power_off_count_down(State *state, SH1106Config *display) {
    int offset_y = 0;
    char buffer[32];
    int margin = 2;
    int row_height = 8 + 2 * margin;

    snprintf(buffer, sizeof buffer, "Powering off in %d...", state->power_off_count_down_sec);
    sh1106_draw_filled_rectangle(display, 0, offset_y,
                                 display->width, row_height);
    sh1106_draw_string_centered_x(display, offset_y + margin,
                                  FONT_SMALL, FONT_BLACK, buffer);
}

void content_motion_sensors_data(const State *state, SH1106Config *display) {
    int offset_x = 0;
    int offset_y = STATUS_BAR_HEIGHT + 5;
    char buffer[20];

    offset_y += 10;
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, "x");
    offset_y += 10;
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, "y");
    offset_y += 10;
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, "z");
    offset_x += 9;
    offset_y = STATUS_BAR_HEIGHT + 5;

    sh1106_draw_string(display, offset_x + 1 * 5, offset_y, FONT_SMALL, FONT_WHITE, " Accel");
    offset_y += 10;
    sprintf(buffer, "%7.2f", state->motion.accel_x);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, "%7.2f", state->motion.accel_y);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, "%7.2f", state->motion.accel_z);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_x += 7 * 5 + 1;
    offset_y += 12;

    sprintf(buffer, "G: %6.2f", sqrt(
        state->motion.accel_x * state->motion.accel_x
            + state->motion.accel_y * state->motion.accel_y
            + state->motion.accel_z * state->motion.accel_z));
    sh1106_draw_string(display, 0, offset_y, FONT_SMALL, FONT_WHITE, buffer);

    snprintf(buffer, sizeof buffer, " Temp: %5.1f%c", state->motion.temperature, SPECIAL_CHAR_DEGREES);
    sh1106_draw_string(display, 11 * 5, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y = STATUS_BAR_HEIGHT + 5;

    sh1106_draw_string(display, offset_x + 2 * 5, offset_y, FONT_SMALL, FONT_WHITE, " Gyro");
    offset_y += 10;
    sprintf(buffer, " %7.1f", state->motion.gyro_x);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, " %7.1f", state->motion.gyro_y);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, " %7.1f", state->motion.gyro_z);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_x += 7 * 5 + 1;
    offset_y = STATUS_BAR_HEIGHT + 5;

    if (!state->motion.has_compass) return;
    sh1106_draw_string(display, offset_x + 2 * 5, offset_y, FONT_SMALL, FONT_WHITE, " Comp");
    offset_y += 10;
    sprintf(buffer, " %7.0f", state->motion.compass_x);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, " %7.0f", state->motion.compass_y);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, " %7.0f", state->motion.compass_z);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
}

void draw_level_circle(SH1106Config *display, int center_x, int center_y, int radius, const Vector3Spherical *vector, double factor) {
    if (radius <= 2) return;

    Vector3Spherical vector_unified = {
        .r = min(radius, vector->r * factor * (radius - 1)),
        .theta = vector->theta,
        .phi = vector->phi,
    };
    Vector3 vector_cartesian = {0};
    spherical_to_cartesian_vectors(vector_unified, &vector_cartesian);

    sh1106_draw_pixel(display, center_x, center_y, FONT_WHITE);
    sh1106_draw_circle(display, center_x, center_y, radius, FONT_WHITE);

    sh1106_draw_circle(display,
                      center_x + (int) round(vector_cartesian.x),
                      center_y + (int) round(vector_cartesian.y),
                      2, FONT_WHITE);
}

void content_motion_sensors_data_graphical(const State *state, SH1106Config *display) {
    char buffer[20];
    int radius = 16;
    int margin = (int) round(display->width / 3.0 - 2.0 * radius);
    int offset_x = margin / 2 + radius;
    int offset_y = STATUS_BAR_HEIGHT + (display->height - STATUS_BAR_HEIGHT - 8) / 2;

    draw_level_circle(display, offset_x, offset_y, radius,
                      &state->motion.accel, 1.0 / 0.4);
    int length = snprintf(buffer, sizeof buffer, "%3.1f", state->motion.accel.r);
    sh1106_draw_string(display, offset_x - font_width * length / 2, display->height - 8,
                       FONT_SMALL, FONT_WHITE, buffer);

    offset_x += margin + 2 * radius;
    draw_level_circle(display, offset_x, offset_y, radius,
                      &state->motion.gyro, 1.0 / 200);
    length = snprintf(buffer, sizeof buffer, "%3.0f", state->motion.gyro.r);
    sh1106_draw_string(display, offset_x - font_width * length / 2, display->height - 8,
                       FONT_SMALL, FONT_WHITE, buffer);

    if (!state->motion.has_compass) return;
    offset_x += margin + 2 * radius;
    draw_level_circle(display, offset_x, offset_y, radius,
                      &state->motion.compass, 1.0 / 40);
    length = snprintf(buffer, sizeof buffer, "%3.0f", state->motion.compass.r);
    sh1106_draw_string(display, offset_x - font_width * length / 2, display->height - 8,
                       FONT_SMALL, FONT_WHITE, buffer);
}

void content_sensors_input(const State *state, SH1106Config *display) {
    int offset_x = 2;
    int offset_y = STATUS_BAR_HEIGHT + 5;
    char buffer[64];

    snprintf(buffer, sizeof buffer, "Pedal: %3.0f %%", state->car.gas_pedal * 100);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);

    offset_y += 10;
    offset_x += 7 * (font_width + 1);
    snprintf(buffer, sizeof buffer, "%4.2f", state->car.gas_pedal_0_volts);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_x += 5 * (font_width + 1);
    snprintf(buffer, sizeof buffer, "%4.2f", state->car.gas_pedal_1_volts);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_x += 5 * (font_width + 1);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, "V");

    offset_x = 2;
    offset_y += 10;
    snprintf(buffer, sizeof buffer, "Buttons: %4d", state->buttons.button0);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_x += 14 * (font_width + 1);
    snprintf(buffer, sizeof buffer, "%4d", state->buttons.button1);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
}

char *content_actions_get_option_text(ActionsScreenOptions option_index) {
    switch (option_index) {
        case ScreenActionsOptions_LockDoors:
            return "Lock doors";
        case ScreenActionsOptions_ActivateSeatbelt:
            return "Set seatbelt on";
        case ScreenActionsOptions_ActivateDiagnostics:
            return "Initialize diagnostics";
        case ScreenActionsOptions_Reboot:
            return "Reboot";
        default:
            return "";
    }
}

void content_actions(const State *state, SH1106Config *display) {
    static int options_start_index = 0;
    const int selection_item_height = 12;
    const int options_total_height = display->height - STATUS_BAR_HEIGHT - 2;
    const int total_displayable_options = options_total_height / selection_item_height;
    char *buffer = NULL;

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
        content_main_menu_option(display, y, selection_item_height, buffer,
                                 state->display.actions_option_selection == options_start_index + i);
    }
}

void content_location_data(const State *state, SH1106Config *display) {
    int offset_x = 1;
    int offset_y;
    char buffer[64];

    // Display GSM state
    offset_y = display->height - 10;
    if (state->gsm.time.year > 2000) {
        snprintf(buffer, sizeof buffer, "%2d:%02d:%02d",
                 state->gsm.time.hours,
                 state->gsm.time.minutes,
                 state->gsm.time.seconds
        );
        sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    }

    offset_x += 9 * (font_width + 1);
    if (state->gsm.time.year > 2000) {
        snprintf(buffer, sizeof buffer, "%2d-%02d",
                 state->gsm.time.day,
                 state->gsm.time.month
        );
        sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    }

    offset_x = display->width - 6 * (font_width + 1);
    if (state->gsm.sim_status != SIM_UNKNOWN) {
        sprintf(buffer, "%c SIM",
                state->gsm.sim_status == SIM_PRESENT ? SPECIAL_CHAR_TICK_MARK : SPECIAL_CHAR_CROSS_MARK);
        sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    }

    offset_x = 1;
    offset_y = STATUS_BAR_HEIGHT + 5;

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
        sh1106_draw_string(display, 1, offset_y + 5, FONT_SMALL, FONT_WHITE, buffer);
        return;
    }

    snprintf(buffer, sizeof buffer, "%2d:%02d:%02d",
             state->location.time.hours,
             state->location.time.minutes,
             state->location.time.seconds
    );
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    snprintf(buffer, sizeof buffer, "%2d-%02d-%04d",
             state->location.time.day,
             state->location.time.month,
             state->location.time.year
    );
    sh1106_draw_string(display, display->width / 2, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;

    snprintf(buffer, sizeof buffer, "%.5lf", state->location.latitude);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    snprintf(buffer, sizeof buffer, "%.5lf", state->location.longitude);
    sh1106_draw_string(display, display->width / 2, offset_y, FONT_SMALL, FONT_WHITE, buffer);

    offset_y += 10;
    snprintf(buffer, sizeof buffer, "Q:%d S:%d E:%d A:%.0lf",
             state->location.quality,
             state->location.satellites,
             state->location.is_effective_positioning,
             state->location.altitude);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);

    offset_y += 10;
    snprintf(buffer, sizeof buffer, "%6.2lf km/h", state->location.ground_speed);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    snprintf(buffer, sizeof buffer, "%3.0lf%c", state->location.ground_heading, SPECIAL_CHAR_DEGREES);
    sh1106_draw_string(display, display->width - 7 * (font_width + 1), offset_y, FONT_SMALL, FONT_WHITE, buffer);
}

void content_activate_diagnostics(const State *state, SH1106Config *display) {
    int offset_y = STATUS_BAR_HEIGHT + 5;

    offset_y += 10;
    switch (state->diagnostics.status) {
        case DiagnosticsStep_IgnitionOff:
            if (state->car.speed > 0 || !state->car.is_parking_brake_on) {
                sh1106_draw_string_centered_x(display, offset_y, FONT_SMALL, FONT_WHITE, "Park the car.");
            } else if (state->car.is_braking) {
                sh1106_draw_string_centered_x(display, offset_y, FONT_SMALL, FONT_WHITE, "Release the brakes.");
            } else {
                sh1106_draw_string_centered_x(display, offset_y, FONT_SMALL, FONT_WHITE, "Turn ignition off.");
            }
            return;
        case DiagnosticsStep_IgnitionOffWait5Sec:
            sh1106_draw_string_centered_x(display, offset_y, FONT_SMALL, FONT_WHITE, "Just a moment...");
            return;
        case DiagnosticsStep_IgnitionOn:
            sh1106_draw_string_centered_x(display, offset_y, FONT_SMALL, FONT_WHITE, "Turn ignition on.");
            offset_y += 15;
            sh1106_draw_string_centered_x(display, offset_y, FONT_SMALL, FONT_WHITE, "Do NOT start the car!");
            return;
        case DiagnosticsStep_ReleasePedal:
            sh1106_draw_string_centered_x(display, offset_y, FONT_SMALL, FONT_WHITE, "Done.");
            return;
        default:
            break;
    }
    offset_y -= 10;

    double total_time = (double) (state->diagnostics.process_estimated_end_time -
        state->diagnostics.process_start_time);
    double current_time = (double) (esp_timer_get_time_ms() - state->diagnostics.process_start_time);
    double progress = min(1.0, max(0, current_time / total_time));

    sh1106_draw_string_centered_x(display, offset_y, FONT_SMALL, FONT_WHITE, "Running procedure...");
    offset_y += 15;

    // Animate progress
    // Draw the container
    int virtual_pedal_container_x = 20;
    int virtual_pedal_container_width = display->width - virtual_pedal_container_x * 2;
    sh1106_draw_horizontal_line(display, virtual_pedal_container_x, offset_y, virtual_pedal_container_width);
    sh1106_draw_horizontal_line(display, virtual_pedal_container_x, offset_y + 5, virtual_pedal_container_width);
    sh1106_draw_vertical_line(display, virtual_pedal_container_x - 1, offset_y + 1, 4);
    sh1106_draw_vertical_line(display, virtual_pedal_container_x + virtual_pedal_container_width, offset_y + 1, 4);

    // Draw the value
    int virtual_pedal_value_width = (int) (progress * (virtual_pedal_container_width - 2));
    int virtual_pedal_value_x = virtual_pedal_container_x + 1;
    sh1106_draw_filled_rectangle(display, virtual_pedal_value_x, offset_y + 2, virtual_pedal_value_width, 2);

    offset_y += 15;


    offset_y += 13;
    sh1106_draw_string_centered_x(display, offset_y, FONT_SMALL, FONT_WHITE, "Do NOT start the car!");
}

void content_about(const State *state, SH1106Config *display) {
    int offset_x = 0;
    int offset_y = STATUS_BAR_HEIGHT + 5;
    char buffer[64];

    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, "SD card file:");
    offset_y += 9;

    if (strlen(state->storage.filename) < display->width / FONT_SMALL) {
        sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, state->storage.filename);
    } else {
        int max_string_length = min(sizeof buffer, display->width / FONT_SMALL + 1);

        strcpy(buffer, state->storage.filename);
        buffer[max_string_length] = '\0';
        sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);

        offset_y += 9;
        strcpy(buffer, state->storage.filename + max_string_length);
        buffer[max_string_length] = '\0';
        sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    }

    offset_y += 9;
    sprintf(buffer, "Session ID: %lu", state->logging_session_id);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);

    offset_y += 12;
    char runtime[32];
    format_time(esp_timer_get_time_ms(), runtime);
    sprintf(buffer, "Runtime: %s", runtime);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
}

void content_about_cruise_control(const State *state, SH1106Config *display) {
    int offset_x = 0;
    int offset_y = STATUS_BAR_HEIGHT + 5;
    char buffer[64];

    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, "Cruise control params");
    offset_y += 10;

    sprintf(buffer, "Kp: %.6f", state->cruise_control.pidKp);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 9;
    sprintf(buffer, "Ki:  %.6f", state->cruise_control.pidKi);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 9;
    sprintf(buffer, "Kd: %.6f", state->cruise_control.pidKd);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 10;
    sprintf(buffer, "Pedal: %3.0f %%", state->car.gas_pedal * 100);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
}

void content_about_car(const State *state, SH1106Config *display) {
    int offset_x = 0;
    int offset_y = STATUS_BAR_HEIGHT + 5;
    char buffer[64];

    sprintf(buffer, "%c Brake", state->car.is_braking ? SPECIAL_CHAR_CIRCLE_CLOSED : SPECIAL_CHAR_CIRCLE_OPEN);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 9;
    sprintf(buffer, "%c Ignition", state->car.is_ignition_on ? SPECIAL_CHAR_CIRCLE_CLOSED : SPECIAL_CHAR_CIRCLE_OPEN);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 9;
    sprintf(buffer, "%c Blower", state->car.is_blower_on ? SPECIAL_CHAR_CIRCLE_CLOSED : SPECIAL_CHAR_CIRCLE_OPEN);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 9;
    sprintf(buffer, "%c Driver door locked",
            state->car.is_drivers_door_locked ? SPECIAL_CHAR_CIRCLE_CLOSED : SPECIAL_CHAR_CIRCLE_OPEN);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 9;
    sprintf(buffer, "%c Other doors locked",
            state->car.is_other_doors_locked ? SPECIAL_CHAR_CIRCLE_CLOSED : SPECIAL_CHAR_CIRCLE_OPEN);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
//    offset_y += 9;
//    sprintf(buffer, "[%c] Parking brake", state->car.is_parking_brake_on ? 'Y' : ' ');
//    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);

    offset_x = display->width / 2;
    offset_y = STATUS_BAR_HEIGHT + 5;
    sprintf(buffer, "%c Locked", state->car.is_locked ? SPECIAL_CHAR_CIRCLE_CLOSED : SPECIAL_CHAR_CIRCLE_OPEN);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);
}

void content_boot_screen(const State *state, SH1106Config *display) {
    int offset_y = STATUS_BAR_HEIGHT + 5;
    char buffer[32];

    offset_y = STATUS_BAR_HEIGHT + (display->height - STATUS_BAR_HEIGHT - 8) / 2 - 10;
    sh1106_draw_string_centered_x(display, offset_y, FONT_SMALL, FONT_WHITE, "Booting...");
    offset_y += 11;
    snprintf(buffer, sizeof buffer, "v%s", APP_VERSION);
    sh1106_draw_string_centered_x(display, offset_y, FONT_SMALL, FONT_WHITE, buffer);

    offset_y += 20;

    // Animate progress
    if (state->boot.max_progress == 0) return;
    double progress = (double) state->boot.progress / state->boot.max_progress;

    // Draw the container
    int virtual_pedal_container_x = 20;
    int virtual_pedal_container_height = 5;
    int virtual_pedal_container_width = display->width - virtual_pedal_container_x * 2;
    sh1106_draw_horizontal_line(display, virtual_pedal_container_x, offset_y, virtual_pedal_container_width);
    sh1106_draw_horizontal_line(display, virtual_pedal_container_x, offset_y + virtual_pedal_container_height - 1,
                                virtual_pedal_container_width);
    sh1106_draw_vertical_line(display, virtual_pedal_container_x - 1, offset_y + 1, virtual_pedal_container_height - 2);
    sh1106_draw_vertical_line(display, virtual_pedal_container_x + virtual_pedal_container_width, offset_y + 1,
                              virtual_pedal_container_height - 2);

    // Draw the value
    int virtual_pedal_value_width = (int) (progress * (virtual_pedal_container_width - 2));
    int virtual_pedal_value_x = virtual_pedal_container_x + 1;
    sh1106_draw_filled_rectangle(display, virtual_pedal_value_x, offset_y + 2, virtual_pedal_value_width,
                                 virtual_pedal_container_height - 4);
}
