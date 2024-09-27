#include <driver/adc.h>
#include "config.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
#include "state.h"
#include "control/control.h"
#include "connectivity/spi.h"


_Noreturn void process_main(State *state) {
    while (1) {
        // Collect data
        control_read_can_bus(state);
        control_read_analog_sensors(state);
        control_read_user_input(state);

        // Process data
        control_cruise_control(state);
        control_car_gear(state);
        control_led_indicator_step(state);
    }
}

void init(State *state) {
    adc1_config_width(ADC_RESOLUTION - 9);
    spi_init(state);
    control_init(state);

    state->is_booting = false;
}

// Running on main core
__attribute__((unused)) void app_main(void) {
    State state = {
            .is_booting = true,
            .power_off_count_down_sec = -1,
    };

    init(&state);
    printf("Init done.\n");

    process_main(&state);
}
