//
// Created by Samuel-Anton Jansen on 2025/11/07.
//

#include "test_display.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../peripherals/display/display.h"
#include "mocks/carcomputer/peripherals/display/sh1106_i2c.h"
#include "mocks/idf/esp_timer.h"
#include "utils/bmp.h"

void update_bitmap() {
    initgraph3();
    clrscr(0); //clears vp0 to black
    setcolor(0, 255, 255, 255); //sets current color to white

    SH1106Config* config = getConfig();

    printf("Width: %d, Height: %d\n", config->width, config->height);
    for (int y = 0; y < config->height; y++) {
        for (int x = 0; x < config->width; x++) {
            char pixel = sh1106_read_pixel(config, x, y);
            if (pixel == FONT_BLACK) continue;
            int destination_y = (y + 1) % config->height;
            putpixel(0, x, destination_y);
        }
    }

    writebmp("../../../display.bmp", 0);
}

void test_display_cruisecontrol() {
    _esp_timer_set_time(1000 * 1000);
    State state = {
            .car.speed = 130,
            .car.acceleration = 1,
            .cruise_control.target_speed = 120,
            .power_off_count_down_sec = -1,
        };

    display_init();

    display_update(&state);

    update_bitmap();
}

void test_display_aboutcar() {
    _esp_timer_set_time(1000 * 1000);
    State state = {
            .power_off_count_down_sec = -1,
            .display.current_screen = Screen_About,
            .display.subscreen.about = ScreenAbout_AboutCar,
            .car.is_ignition_on = true,
            .car.is_braking = true,
            .storage.is_connected = true,
        };
    sprintf(state.storage.filename, "file.csv");

    printf("Letter: %c = %d\n", 'B', (int)'B');

    display_init();

    display_update(&state);

    update_bitmap();
}
