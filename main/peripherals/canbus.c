//
// Created by samuel on 19-7-22.
//

#include "../config.h"
#include "canbus.h"
#include "mcp2515_wrapper.h"
#include "../return_codes.h"
#include "../utils.h"
#include <stdint.h>
#include <driver/gpio.h>
#include <esp_timer.h>

void handle_speed_message(State *state, CanMessage *message) {
    if (message->length != 8) {
        return;
    }

    int value = message->data[0] << 8 | message->data[1];
    state->car.speed = value / 96.0;
}

void handle_rpm_message(State *state, CanMessage *message) {
    if (message->length != 8) {
        return;
    }

    uint16_t value = message->data[0] << 8 | message->data[1];
    state->car.rpm = value / 8.0;
    if (value > 150) {
        state->car.rpm += 150;
    }
}

void handle_brake_message(State *state, CanMessage *message) {
    if (message->length != 8) {
        return;
    }

    state->car.is_braking = message->data[6] & 16;
}

void handle_ignition_message(State *state, CanMessage *message) {
    if (message->length != 8) {
        return;
    }

    state->car.is_ignition_on = message->data[0] & 2;
}

void handle_odometer_message(State *state, CanMessage *message) {
    if (message->length != 8) {
        return;
    }

    state->car.odometer_end = (uint32_t) message->data[1] << 16
                              | (uint32_t) message->data[2] << 8
                              | message->data[3];

    if (state->car.odometer_start == 0) {
        state->car.odometer_start = state->car.odometer_end;
    }
}

int message_available() {
    // If CAN_INT pin is low, read receive buffer
    return !gpio_get_level(CANBUS_INTERRUPT_PIN);
}

int read_message(CanMessage *message) {
    return mcp2515_read_message(message);
}

void handle_message(State *state, CanMessage *message) {
    switch (message->id) {
        case 385:
            state->car.last_can_message_time = esp_timer_get_time_ms();
            handle_rpm_message(state, message);
            break;
        case 640:
            state->car.last_can_message_time = esp_timer_get_time_ms();
            handle_ignition_message(state, message);
            break;
        case 852:
            state->car.last_can_message_time = esp_timer_get_time_ms();
            handle_speed_message(state, message);
            handle_brake_message(state, message);
            break;
        case 1477:
            state->car.last_can_message_time = esp_timer_get_time_ms();
            handle_odometer_message(state, message);
            break;
        default:
            break;
    }
}

void canbus_check_messages(State *state) {
    for (int i = 0; i < 10 && message_available(); i++) {
        CanMessage message = {};
        if (read_message(&message) != RESULT_OK) {
            break;
        }
        handle_message(state, &message);
    }
}

void canbus_init(State *state) {
    printf("[CAN] Initializing CAN bus...\n");

    gpio_set_direction(CANBUS_INTERRUPT_PIN, GPIO_MODE_INPUT);
    mcp2515_init();

    printf("[CAN] Init done\n");
}

void canbus_check_controller_connection(State *state) {
    static int64_t last_check_time = 0;
    if (esp_timer_get_time_ms() < last_check_time + CAR_CAN_CONTROLLER_CHECK_INTERVAL) return;
    last_check_time = esp_timer_get_time_ms();

    uint8_t status = mcp2515_get_mode();
    state->car.is_controller_connected = status == 0x60;    // Listen only mode

    if (state->car.is_controller_connected) return;
    canbus_init(state);
}
