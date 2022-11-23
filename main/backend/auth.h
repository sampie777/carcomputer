//
// Created by samuel on 18-11-22.
//

#ifndef CARCOMPUTER_AUTH_H
#define CARCOMPUTER_AUTH_H

#include "../state.h"

void store_access_token(const char *value);

void request_registration_token(State *state);

void poll_registration_status(State *state);

void auth_process(State *state);

void auth_init(State *state);

#endif //CARCOMPUTER_AUTH_H
