//
// Created by samuel on 19-7-22.
//

#include "canbus.h"
#include "../config.h"
#include <stdint.h>
#include <driver/gpio.h>
#include <esp_timer.h>

typedef struct {
    unsigned long id;
    uint8_t length;
    uint8_t data[8];
} CanMessage;

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

void read_message(CanMessage *message) {
//    can_controller.readMsgBuf(&message->id, &message->length, &message->data);
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
        CanMessage message;
        read_message(&message);
        handle_message(state, &message);
    }
}

void canbus_init() {
    printf("[CAN] Initializing CAN bus...");

    printf("[CAN] Init done");
}