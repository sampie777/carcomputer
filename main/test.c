#include <stdio.h>
#include <stdlib.h>

#include "state.h"
#include "control/cruise_control.h"
#include "utils.h"
#include "control/control.h"
#include "test/mocks/idf/esp_timer.h"
#include "test/mocks/carcomputer/peripherals/gas_pedal.h"

#define CAR_MASS (1200.0)
#define FORCE_FACTOR (16500.0)
#define CAR_VELOCITY_RANDOMNESS (0.0)
#define STEP (100)
#define RUN_TIME (40000)

const char* file_name = "../../../test1.csv";

int write_csv() {
    FILE* file = fopen(file_name, "w");
    if (file == NULL) {
        printf("ERROR: Could not open file %s for writing\n", file_name);
        return 1;
    }

    fwrite("", sizeof(char), 0, file);
    fclose(file);
    return 0;
}

int append_csv(const char* data, unsigned long size) {
    if (!data) {
        printf("ERROR: Input BMP_File pointer is NULL\n");
        return 1;
    }

    FILE* file = fopen(file_name, "a+");
    if (file == NULL) {
        printf("ERROR: Could not open file %s for writing\n", file_name);
        return 1;
    }

    fwrite(data, sizeof(char), size, file);
    fclose(file);
    printf("%s", data);
    return 0;
}

void step_time() { _esp_timer_set_time((esp_timer_get_time_ms() + STEP) * 1000); }

double friction = 0;
void simulate_car_step(State* state, int32_t delta) {
    double v = state->car.speed / 3.6;
    double m = CAR_MASS;
    double g = 9.81;
    double A = 3;       // Front area
    double u = 0.1;     // Friction coefficient
    double P0 = 3750;
    double p = 1.3;
    double c = 0.27;
    friction = v < 0.1 && v > -0.1 ? 0 : P0 / v + u * m * g + c * p * A * v * v / 2.0;

    double engineForce = (_get_gas_penal_enabled() ? state->cruise_control.virtual_gas_pedal : state->car.gas_pedal) * FORCE_FACTOR;
    // F = m*a -> a = F / m
    double acceleration = (engineForce - friction)
         / CAR_MASS;
    state->car.speed += acceleration * (delta / 1000.0) + (random_d() - 0.5) * CAR_VELOCITY_RANDOMNESS * (delta / 1000.0);

    state->car.estimated_gear = Gear1;
}

void test_cruise_control(void) {
    write_csv();
    double max_speed = 0;
    double min_speed = 100;

    State state = {0};
    state.boot.is_booting = true;
    state.power_off_count_down_sec = -1;
    state.cruise_control.pidKp = CRUISE_CONTROL_PID_Kp;
    state.cruise_control.pidKi = CRUISE_CONTROL_PID_Ki;
    state.cruise_control.pidKd = CRUISE_CONTROL_PID_Kd;
    state.device_name = "Default";
    state.location.time.timezone = 2; // GMT+2
    state.motion.bias.x = 1.1;
    state.motion.bias.y = -0.04;
    state.motion.bias.z = 0.01;

    state.car.gas_pedal_connected = true;
    state.car.is_connected = true;
    state.car.is_braking = false;
    state.car.estimated_gear = Gear1;
    state.car.is_parking_brake_on = false;

    state.car.speed = 50;
    state.car.gas_pedal = 0;
    state.cruise_control.enabled = true;
    cruise_control_step(&state);
    state.car.speed = 35;

    char buffer[512];
    sprintf(buffer, "resp_timer_get_time_ms();"
            "state.car.gas_pedal;"
            "state.car.acceleration;"
            "friction;"
            "state.car.speed;"
            "state.cruise_control.virtual_gas_pedal;"
            "state.cruise_control.control_value;"
            "state.cruise_control.target_speed;"
            "state.cruise_control.error;"
            "state.cruise_control.integral;"
            "state.cruise_control.derivative;"
            "state.cruise_control.enabled"
            "\n");
    append_csv(buffer, strlen(buffer));

    for (int i = 0; i < RUN_TIME / STEP; i++) {
        simulate_car_step(&state, STEP);

        control_cruise_control(&state);
        control_process_car(&state);

        sprintf(buffer, "%lld;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%d\n",
                esp_timer_get_time_ms(),
                state.car.gas_pedal,
                state.car.acceleration,
                friction,
                state.car.speed,
                state.cruise_control.virtual_gas_pedal,
                state.cruise_control.control_value,
                state.cruise_control.target_speed,
                state.cruise_control.error,
                state.cruise_control.integral,
                state.cruise_control.derivative,
                state.cruise_control.enabled
        );
        string_char_replace(buffer, '.', ',');
        append_csv(buffer, strlen(buffer));

        step_time();

        max_speed = max(max_speed, state.car.speed);
        min_speed = min(min_speed, state.car.speed);
    }

    printf("Max speed: %lf\n", max_speed);
    printf("Min speed: %lf\n", min_speed);
}

void test_hourly_eta() {
    State state = {0};
    state.car.speed = 50;
    state.cruise_control.target_speed = 100;

    double hourly_eta_deviation = cruise_control_calculate_hour_eta_deviation_for_curren_speed(&state);
    char buffer[32];
    format_time_h_mm((int64_t) (hourly_eta_deviation * 3600 * 1000), buffer);
    printf("%s\n", buffer);
}

int main(void) {
    test_cruise_control();
    // test_hourly_eta();
    return 0;
}
