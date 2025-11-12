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
#include "utils/common.h"

#define STEP (100)
#define RUN_TIME (1000)


void update_bitmap() {
    static int i = 0;
    initgraph3();
    setcolor(0, 255, 255, 255); //sets current color to white

    SH1106Config *config = getConfig();

    for (int y = 0; y < config->height; y++) {
        for (int x = 0; x < config->width; x++) {
            char pixel = sh1106_read_pixel(config, x, y);
            if (pixel == FONT_BLACK) continue;
            int destination_y = (y + 1) % config->height;
            int destination_x = y == config->height - 1 ? x + 1 : x;;
            putpixel(0, destination_x, destination_y);
        }
    }

    char filename[128];
    snprintf(filename, sizeof(filename), "../../../test_output/screen%d.bmp", i++);
    writebmp(filename, 0);
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
        // .display.current_screen = Screen_About,
        // .display.subscreen.about = ScreenAbout_AboutCar,
        .car.is_ignition_on = true,
        .car.is_braking = true,
        .storage.is_connected = true,
        .car.speed = 100,
        .cruise_control.target_speed = 120,
        // .cruise_control.enabled = true,
        .cruise_control.virtual_gas_pedal = 0.3,

        // .boot = {
        //     .is_booting = true,
        //     .max_progress = 10,
        //     .progress = 3,
        // },
    };
    sprintf(state.storage.filename, "file.csv");

    display_init();
    SH1106Config *config = getConfig();

    for (int i = 0; i < RUN_TIME / STEP; i++) {
        state.car.speed += 1;
        display_update(&state);

        update_bitmap();
        step_time();
    }
}
