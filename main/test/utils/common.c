//
// Created by Samuel-Anton Jansen on 2025/11/12.
//

#include "common.h"

#include <stdio.h>

#include "bmp.h"
#include "../mocks/idf/esp_timer.h"
#include "../../utils.h"
#include "../mocks/carcomputer/peripherals/gas_pedal.h"
#include "../mocks/carcomputer/peripherals/display/sh1106_i2c.h"

void step_time(uint64_t delta) { _esp_timer_set_time((esp_timer_get_time_ms() + delta) * 1000); }

#define CAR_MASS (1000.0)
#define FORCE_FACTOR (16500.0)
#define CAR_VELOCITY_RANDOMNESS (0.0)

void simulate_car_step(State* state, int32_t delta) {
    double v = state->car.speed / 3.6;
    double m = CAR_MASS;
    double g = 9.81;
    double A = 3;       // Front area
    double u = 0.03;     // Friction coefficient
    double P0 = 3750;
    double p = 1.3;
    double c = 0.27;
    double friction = v < 0.1 && v > -0.1 ? 0 : P0 / v + u * m * g + c * p * A * v * v / 2.0;

    double engineForce = (_get_gas_penal_enabled() ? state->speed_control.virtual_gas_pedal : state->car.gas_pedal) * FORCE_FACTOR;
    // F = m*a -> a = F / m
    double acceleration = (engineForce - friction) / CAR_MASS;
    // a = dv/dt -> dv = a*dt
    double speed_increase = acceleration + (random_d() - 0.5) * CAR_VELOCITY_RANDOMNESS;
    state->car.speed += speed_increase * (delta / 1000.0);

    state->car.estimated_gear = Gear1;
}

void update_bitmap() {
    static int i = 0;
    BmpImage bmp = {
        .width = 128,
        .height = 64,
    };
    bmp_init(&bmp);

    SH1106Config *config = getConfig();

    for (int y = 0; y < config->height; y++) {
        for (int x = 0; x < config->width; x++) {
            char pixel = sh1106_read_pixel(config, x, y);
            if (pixel == FONT_BLACK) continue;
            int destination_y = (y + 1) % config->height;
            int destination_x = y == config->height - 1 ? x + 1 : x;;
            bmp_draw_pixel(&bmp, destination_x, destination_y);
        }
    }

    char filename[128];
    snprintf(filename, sizeof(filename), "../../../test_output/screen%05d.bmp", i++);
    bmp_save(&bmp, filename);
}