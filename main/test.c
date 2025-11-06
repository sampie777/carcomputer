#include <stdio.h>

#include "state.h"
#include "control/cruise_control.h"
#include <esp_timer.h>

#include "utils.h"

int main(void) {
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
    state.car.speed = 45;

    for (int i = 0; i < 20; i++) {
        cruise_control_step(&state);
        printf("%lf %lf | %lf %lf %lf %lld\n",
               state.car.acceleration,
               state.car.gas_pedal,
               state.cruise_control.target_speed,
               state.cruise_control.control_value,
               state.cruise_control.virtual_gas_pedal,
               esp_timer_get_time_ms()
        );

        _esp_timer_set_time(esp_timer_get_time() + 100 * 1000);
    }

    printf("Hello, World! %d\n", state.logging_session_id);
    return 0;
}
