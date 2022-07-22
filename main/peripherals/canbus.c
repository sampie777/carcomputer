//
// Created by samuel on 19-7-22.
//

#include "../config.h"
#include "canbus.h"
#include "mcp2515_wrapper.h"
#include "../return_codes.h"
#include <stdint.h>
#include <driver/gpio.h>
#include <esp_timer.h>

void handleSpeedMessage(State *state, CanMessage *message) {
    if (message->length != 8) {
        return;
    }

    int value = message->data[0] << 8 | message->data[1];
    state->car.speed = value / 96.0;
}

void handleRpmMessage(State *state, CanMessage *message) {
    if (message->length != 8) {
        return;
    }

    uint16_t value = message->data[0] << 8 | message->data[1];
    state->car.rpm = value / 8.0;
    if (value > 150) {
        state->car.rpm += 150;
    }
}

void handleBrakeMessage(State *state, CanMessage *message) {
    if (message->length != 8) {
        return;
    }

    state->car.isBraking = message->data[6] & 16;
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
            state->car.last_can_message_time = esp_timer_get_time();
            handleRpmMessage(state, message);
            break;
        case 852:
            state->car.last_can_message_time = esp_timer_get_time();
            handleSpeedMessage(state, message);
            handleBrakeMessage(state, message);
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

    state->car.connected = true;
    printf("[CAN] Init done\n");
}