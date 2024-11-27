//
// Created by samuel on 9-12-22.
//

#include <math.h>
#include "display_screens.h"
#include "display.h"
#include "../../utils.h"


void content_server_registration(State *state, SH1106Config *sh1106) {
    int offset_x = 5;
    int offset_y = STATUS_BAR_HEIGHT + 5;

    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, "Please visit:");
    offset_y += 10;

    char *buffer = "car.sajansen.nl";
    sh1106_draw_string(sh1106, (int) (sh1106->width - strlen(buffer) * 5) / 2, offset_y, FONT_SMALL, FONT_WHITE, buffer);
    offset_y += 14;

    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_SMALL, FONT_WHITE, "Token:");
    offset_y += 11;
}

void content_cruise_control(State *state, SH1106Config *sh1106) {
    int offset_x = 5;
    int offset_y = STATUS_BAR_HEIGHT + 17;
    char buffer[20];
    sprintf(buffer, "%3.0f%s ", state->car.speed, state->cruise_control.enabled ? "/" : " km/h");
    offset_x += sh1106_draw_string(sh1106, offset_x, offset_y, FONT_MEDIUM, FONT_WHITE, buffer);

    if (!state->cruise_control.enabled) return;

    sprintf(buffer, "%.0f", state->cruise_control.target_speed);
    sh1106_draw_string(sh1106, offset_x, offset_y, FONT_LARGE, FONT_WHITE, buffer);

    // Animate virtual pedal position
    // Draw the container
    int virtual_pedal_container_y = STATUS_BAR_HEIGHT + 6;
    int virtual_pedal_container_height = sh1106->height - virtual_pedal_container_y - 10;
    sh1106_draw_vertical_line(sh1106, sh1106->width - 5, virtual_pedal_container_y, virtual_pedal_container_height);
    sh1106_draw_vertical_line(sh1106, sh1106->width - 1, virtual_pedal_container_y, virtual_pedal_container_height);
    sh1106_draw_horizontal_line(sh1106, sh1106->width - 4, virtual_pedal_container_y - 1, 3);
    sh1106_draw_horizontal_line(sh1106, sh1106->width - 4, virtual_pedal_container_y + virtual_pedal_container_height, 3);

    // Draw the value
    int virtual_pedal_value_height = (int) (state->cruise_control.virtual_gas_pedal * virtual_pedal_container_height);
    int virtual_pedal_value_y = virtual_pedal_container_y + virtual_pedal_container_height - virtual_pedal_value_height;
    sh1106_draw_filled_rectangle(sh1106, sh1106->width - 4 + 1, virtual_pedal_value_y, 2, virtual_pedal_value_height);
}

void content_power_off_count_down(State *state, SH1106Config *sh1106) {
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

void draw_check_box(SH1106Config *sh1106, int x, int y, int size, bool checked) {
    sh1106_draw_rectangle(sh1106, x, y, size, size);
    if (!checked) return;
    sh1106_draw_filled_rectangle(sh1106, x + 2, y + 2, max(0, size - 2 * 2), max(0, size - 2 * 2));
}

void content_main_menu_option(SH1106Config *sh1106, int y, int height, const char *text, bool highlighted) {
    if (highlighted) {
        sh1106_draw_filled_rectangle(sh1106, 0, y,
                                     sh1106->width, height);
    }
    sh1106_draw_string(sh1106, 5, y + (height - 8) / 2,
                       FONT_SMALL, highlighted ? FONT_BLACK : FONT_WHITE, text);
}

char *content_main_menu_get_option_text(ScreenMenuOptions option_index) {
    switch (option_index) {
        case ScreenMenuOption_CruiseControl:
            return "Cruise control";
        default:
            return "";
    }
}

void content_main_menu(const State *state, SH1106Config *sh1106) {
    static int options_start_index = 0;
    const int selection_item_height = 12;
    const int options_total_height = sh1106->height - STATUS_BAR_HEIGHT - 2;
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
        content_main_menu_option(sh1106, y, selection_item_height, buffer, state->display.menu_option_selection == options_start_index + i);
    }
}