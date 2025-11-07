//
// Created by samuel on 19-7-22.
//

#include "canbus.h"
#include <driver/gpio.h>

int canbus_send(const CanMessage *message) {
    return 0;
}

int canbus_send_lock_doors(const State *state, bool lock_doors) {
    return 0;
}

int canbus_send_seatbelt_message(const State *state, bool set_seatbelt_on) {
    return 0;
}

void canbus_init() {}
void canbus_check_messages(State *state) {}
void canbus_check_controller_connection(State *state) {}