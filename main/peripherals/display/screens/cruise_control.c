//
// Created by Samuel-Anton Jansen on 2025/11/19.
//

#include "cruise_control.h"

#include <stdio.h>
#include <math.h>
#include "../display_screens.h"
#include "../display.h"
#include "../../../utils.h"
#include "../special_chars.h"
#include "../../../control/cruise_control.h"


void content_cruise_control(State *state, SH1106Config *display) {
    int offset_x = 5;
    int offset_y = STATUS_BAR_HEIGHT + 10;
    char buffer[32];
    snprintf(buffer, sizeof buffer, "%3.0f%s ", state->car.speed, state->speed_control.cruise_control.enabled ? "/" : " km/h");
    offset_x += sh1106_draw_string(display, offset_x, offset_y, FONT_MEDIUM, FONT_WHITE, buffer);

    if (state->speed_control.cruise_control.enabled) {
        sprintf(buffer, "%.0f", state->speed_control.cruise_control.target_speed);
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


    offset_x = 25;
    sprintf(buffer, "%.2f m/s%c", state->car.acceleration, SPECIAL_CHAR_POWER2);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);

    if (!state->speed_control.cruise_control.enabled) return;

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
    int virtual_pedal_value_height = (int) (state->speed_control.virtual_gas_pedal * virtual_pedal_container_height);
    int virtual_pedal_value_y = virtual_pedal_container_y + virtual_pedal_container_height - virtual_pedal_value_height;
    sh1106_draw_filled_rectangle(display, display->width - 4 + 1, virtual_pedal_value_y, 2, virtual_pedal_value_height);
}

void content_cruise_control_graph(State *state, SH1106Config *display) {
    int offset_y = STATUS_BAR_HEIGHT + 3;

    // Draw vertical axis
    sh1106_draw_vertical_line(display, 0, offset_y, display->height - offset_y);
    sh1106_draw_horizontal_line(display, 0, offset_y, 3);
    sh1106_draw_horizontal_line(display, 0, offset_y + (display->height - 1 - offset_y) / 2, 3);
    sh1106_draw_horizontal_line(display, 0, display->height - 1, 3);

    // Draw data
    for (int i = 2; i < CRUISE_CONTROL_GRAPH_SIZE; i++) {
        if (state->speed_control.cruise_control.graph.virtual_gas_pedal[i] < 0) continue;
        int y = (int) round((display->height - offset_y) * (1.0 - state->speed_control.cruise_control.graph.virtual_gas_pedal[i] / 100.0));
        sh1106_draw_pixel(display, i, y + offset_y - 1, FONT_WHITE);
    }

    char buffer[32];
    snprintf(buffer, sizeof buffer, "%3d %% pedal", (int) (state->speed_control.virtual_gas_pedal * 100.0));
    sh1106_draw_string(display, 7, STATUS_BAR_HEIGHT + 5, FONT_SMALL, FONT_WHITE, buffer);
}

void content_cruise_control_eta(State *state, SH1106Config *display) {
    int offset_y = STATUS_BAR_HEIGHT + 10;
    char buffer[32];

    snprintf(buffer, sizeof buffer, "%3.0f/ %.0f", state->speed_control.cruise_control.eta_target_speed, state->speed_control.cruise_control.target_speed);
    sh1106_draw_string(display, 5, offset_y, FONT_MEDIUM, FONT_WHITE, buffer);
    offset_y += 8 * FONT_MEDIUM + 4;

    if (state->speed_control.cruise_control.target_speed < 1) return;

    double hourly_eta_deviation = state->speed_control.cruise_control.target_speed / state->speed_control.cruise_control.eta_target_speed;

    // Limit
    double limited_hourly_eta_deviation = max(0.1, min(2.0, hourly_eta_deviation));

    int64_t milliseconds = (int64_t) ((limited_hourly_eta_deviation - 1) * 3600 * 1000);
    bool is_negative = milliseconds < 0;
    if (is_negative) milliseconds *= -1;
    char time_buffer[16];
    if (hourly_eta_deviation > 2 || hourly_eta_deviation < 0.1) {
        snprintf(time_buffer, sizeof time_buffer, " >1h");
    } else {
        format_time_h_mm(milliseconds, time_buffer);
    }

    snprintf(buffer, sizeof buffer, "1h ETA %c", SPECIAL_CHAR_ARROW_RIGHT);
    sh1106_draw_string(display, 5, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 11;
    snprintf(buffer, sizeof buffer, "%c%s", is_negative ? '-' : '+', time_buffer);
    sh1106_draw_string_centered_x(display, offset_y, FONT_MEDIUM, FONT_WHITE, buffer);
}

void content_pedal_control(State *state, SH1106Config *display) {
    int offset_x = 5;
    int offset_y = STATUS_BAR_HEIGHT + 10;
    char buffer[32];
    snprintf(buffer, sizeof buffer, "%3.0f%s ", state->car.gas_pedal * 100, state->speed_control.pedal_control.enabled ? "/" : " %");
    offset_x += sh1106_draw_string(display, offset_x, offset_y, FONT_MEDIUM, FONT_WHITE, buffer);

    if (state->speed_control.pedal_control.enabled) {
        sprintf(buffer, "%.0f", state->speed_control.virtual_gas_pedal * 100);
        offset_x += sh1106_draw_string(display, offset_x, offset_y, FONT_LARGE, FONT_WHITE, buffer);
        sh1106_draw_string(display, offset_x, offset_y, FONT_MEDIUM, FONT_WHITE, "%");
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


    offset_x = 25;
    sprintf(buffer, "%.2f m/s%c", state->car.acceleration, SPECIAL_CHAR_POWER2);
    sh1106_draw_string(display, offset_x, offset_y, FONT_SMALL, FONT_WHITE, buffer);

    if (!state->speed_control.pedal_control.enabled) return;

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
    int virtual_pedal_value_height = (int) (state->speed_control.virtual_gas_pedal * virtual_pedal_container_height);
    int virtual_pedal_value_y = virtual_pedal_container_y + virtual_pedal_container_height - virtual_pedal_value_height;
    sh1106_draw_filled_rectangle(display, display->width - 4 + 1, virtual_pedal_value_y, 2, virtual_pedal_value_height);
}
