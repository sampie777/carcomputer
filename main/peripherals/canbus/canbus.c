//
// Created by samuel on 19-7-22.
//

#include "../../config.h"
#include "canbus.h"
#include "mcp2515_wrapper.h"
#include "../../return_codes.h"
#include "../../utils.h"
#include "can_definitions.h"
#include <stdint.h>
#include <driver/gpio.h>
#include <esp_timer.h>

void print_can_message(const CanMessage* message) {
    printf("CAN id=%lu [ ", message->id);
    for (int i = 0; i < message->length; i++) {
        printf("%02x ", message->data[i]);
    }
    printf("]\n");
}

void handle_speed_and_brake_message(State* state, CanMessage* message) {
    if (message->length != CAN_LENGTH_SPEED_AND_BRAKE) {
        return;
    }

    int value = message->data[0] << 8 | message->data[1];
    state->car.speed = value / CAN_SPEED_CALIBRATION;

    state->car.is_braking = message->data[6] & 16;
}

void handle_rpm_message(State* state, CanMessage* message) {
    if (message->length != CAN_LENGTH_RPM) {
        return;
    }

    state->car.rpm_raw = message->data[0] << 8 | message->data[1];
    state->car.rpm = state->car.rpm_raw / CAN_RPM_CALIBRATION;
}

void handle_ignition_message(State* state, CanMessage* message) {
    if (message->length != CAN_LENGTH_IGNITION) {
        return;
    }

    // The next bit is only set/available when the ignition is on. Otherwise, it will always be 0.
    if (state->car.is_ignition_on) {
        state->car.is_seatbelt_on = !(message->data[0] >> CAN_IGNITION_DRIVERS_SEATBELT_BIT) & 1;
    }

    int value = message->data[4] << 8 | message->data[5];
    state->car.speed = value / CAN_SPEED_CALIBRATION;
}

void handle_odometer_message(State* state, CanMessage* message) {
    if (message->length != CAN_LENGTH_ODOMETER) {
        return;
    }

    state->car.odometer = (uint32_t) message->data[1] << 16
        | (uint32_t) message->data[2] << 8
        | message->data[3];

    if (state->car.odometer_start == 0) {
        state->car.odometer_start = state->car.odometer;
    }

    // The next bit is only set/available when the ignition is on. Otherwise, it will always be 0.
    if (state->car.is_ignition_on) {
        state->car.is_parking_brake_on = (message->data[0] >> CAN_ODOMETER_PARKING_BRAKE_BIT) & 1;
    }
}

void handle_door_lock_message(State* state, const CanMessage* message) {
    if (message->length != CAN_LENGTH_DOOR_LOCKS) {
        return;
    }

    state->car.is_blower_on = (message->data[1] >> CAN_DOOR_LOCKS_BLOWER_BIT) & 1;
    state->car.is_drivers_door_locked = (message->data[5] >> CAN_DOOR_LOCKS_DRIVER_DOOR_STATUS_BIT) & 1;
    state->car.is_other_doors_locked = (message->data[5] >> CAN_DOOR_LOCKS_OTHER_DOORS_STATUS_BIT) & 1;
}

void handle_gear_and_lights_message(State* state, const CanMessage* message) {
    if (message->length != CAN_LENGTH_GEAR_LIGHTS) {
        return;
    }

    state->car.is_ignition_on = !((message->data[6] >> CAN_GEAR_LIGHTS_IGNITION_BIT) & 1);
    state->car.is_in_reverse = (message->data[6] >> CAN_GEAR_LIGHTS_REVERSE_BIT) & 1;
    state->car.is_locked = (message->data[2] >> CAN_GEAR_LIGHTS_IS_LOCKED_BIT) & 1;
}

int message_available() {
    // If CAN_INT pin is low, read receive buffer
    return !gpio_get_level(CANBUS_INTERRUPT_PIN);
}

int read_message(CanMessage* message) {
    return mcp2515_read_message(message);
}

void handle_message(State* state, CanMessage* message) {
    switch (message->id) {
        case CAN_ID_RPM:
            handle_rpm_message(state, message);
            break;
        case CAN_ID_IGNITION:
            handle_ignition_message(state, message);
            break;
        case CAN_ID_SPEED_AND_BRAKE:
            handle_speed_and_brake_message(state, message);
            break;
        case CAN_ID_ODOMETER:
            handle_odometer_message(state, message);
            break;
        case CAN_ID_DOOR_LOCKS:
            handle_door_lock_message(state, message);
            break;
        case CAN_ID_GEAR_LIGHTS:
            handle_gear_and_lights_message(state, message);
            break;
        default:
            return;
    }
    state->car.last_can_message_time = esp_timer_get_time_ms();
}

void canbus_check_messages(State* state) {
    for (int i = 0; i < 10 && message_available(); i++) {
        CanMessage message = {};
        int result = read_message(&message);
        if (result == RESULT_EMPTY) break;
        if (result != RESULT_OK) continue;

        handle_message(state, &message);

        if (i >= 9) printf("Too much CAN messages to handle.\n");
    }
}

int canbus_send(const CanMessage* message) {
    return mcp2515_send_message(message);
}

void canbus_init(State* state) {
    printf("[CAN] Initializing CAN bus...\n");

    gpio_set_direction(CANBUS_INTERRUPT_PIN, GPIO_MODE_INPUT);
    mcp2515_init(false);

    printf("[CAN] Init done\n");
}

void canbus_check_controller_connection(State* state) {
    static int64_t last_check_time = 0;
    if (esp_timer_get_time_ms() < last_check_time + CAR_CAN_CONTROLLER_CHECK_INTERVAL) return;
    last_check_time = esp_timer_get_time_ms();

    uint8_t config3 = mcp2515_get_config3();
    state->car.is_controller_connected = (config3 >> 3) == 0x10; // Check certain bits we know will be constant

    if (state->car.is_controller_connected) return;
    canbus_init(state);
}

int canbus_send_lock_doors(const State* state, bool lock_doors) {
    CanMessage message = {
            .id = CAN_ID_DOOR_LOCKS,
            .length = CAN_LENGTH_DOOR_LOCKS,
            .data = {
                0,
                state->car.is_blower_on << CAN_DOOR_LOCKS_BLOWER_BIT,
                0,
                lock_doors
                    ? (CAN_DOOR_LOCKS_LOCK_DRIVER_DOOR | CAN_DOOR_LOCKS_LOCK_OTHER_DOORS)
                    : (CAN_DOOR_LOCKS_UNLOCK_DRIVER_DOOR | CAN_DOOR_LOCKS_UNLOCK_OTHER_DOORS),
                1,
                (state->car.is_drivers_door_locked << CAN_DOOR_LOCKS_DRIVER_DOOR_STATUS_BIT)
                | (state->car.is_other_doors_locked << CAN_DOOR_LOCKS_OTHER_DOORS_STATUS_BIT),
                0,
                0
            }
        };

    printf("Locking doors... ");
    int result = canbus_send(&message);
    if (result !=RESULT_OK) printf("failed");
    printf("\n");
    return result;
}

int canbus_generate_speed_and_brake_message(double speed, bool is_braking, bool is_ignition_on) {
    int raw_speed = (int) (speed * CAN_SPEED_CALIBRATION);
    CanMessage message = {
            .id = CAN_ID_SPEED_AND_BRAKE,
            .length = CAN_LENGTH_SPEED_AND_BRAKE,
            .data = {
                raw_speed >> 8,
                raw_speed & 0xff,
                is_ignition_on ? 0xff : 0x00,
                is_ignition_on ? 0xff : 0x00,
                0x40,
                0x00,
                is_braking ? 0x14 : 0x04,
                0x00
            }
        };
    return canbus_send(&message);
}

int canbus_generate_rpm_message(double rpm, double pedal) {
    int raw_rpm = (int) (rpm * CAN_RPM_CALIBRATION);
    CanMessage message = {
            .id = CAN_ID_RPM,
            .length = CAN_LENGTH_RPM,
            .data = {
                raw_rpm >> 8,
                raw_rpm & 0xff,
                0x29,
                (int) (pedal / (224.0 / 255)),
                pedal > 0 ? 0x50 : 0x4f,
                0x20,
                0x00,
                0x00
            }
        };
    return canbus_send(&message);
}

int canbus_generate_ignition_message(double speed, bool is_ignition_on) {
    int raw_speed = (int) (speed * CAN_SPEED_CALIBRATION);
    CanMessage message = {
            .id = CAN_ID_IGNITION,
            .length = CAN_LENGTH_IGNITION,
            .data = {
                is_ignition_on ? 0x02 : 0x00,
                0x00,
                0x00,
                0x00,
                raw_speed >> 8,
                raw_speed & 0xff,
                0x00,
                is_ignition_on ? 0x21 : 0x01,
            }
        };
    return canbus_send(&message);
}
