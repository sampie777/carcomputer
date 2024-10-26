//
// Created by samuel on 19-7-22.
//

#ifndef APP_TEMPLATE_CANBUS_H
#define APP_TEMPLATE_CANBUS_H

#include <stdint.h>
#include "../../state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned long id;
    uint8_t length;
    uint8_t data[8];
} CanMessage;

void canbus_init(State *state);

void canbus_check_messages(State *state);

void canbus_check_controller_connection(State *state);

int canbus_send(const CanMessage *message);

int canbus_send_lock_doors(const State *state, bool lock_doors);

int canbus_generate_speed_and_brake_message(double speed, bool is_braking, bool is_ignition_on);

int canbus_generate_rpm_message(double rpm, double pedal);

int canbus_generate_ignition_message(double speed, bool is_ignition_on);

#ifdef __cplusplus
}
#endif

#endif //APP_TEMPLATE_CANBUS_H
