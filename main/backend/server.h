//
// Created by samuel on 4-8-22.
//

#ifndef CARCOMPUTER_SERVER_H
#define CARCOMPUTER_SERVER_H

#include "../state.h"

typedef struct {
    int code;
    int content_length;
    char *message;
} HttpResponseMessage;

void server_init(State *state);

void server_process(State *state);

bool server_is_ready_for_connections(State *state);

int server_send_trip_end(State *state);

int server_send_data_log_record(State *state);

int server_send_data(State *state, const char *url, const char *json, uint8_t wifi_only);

int server_receive_data(State *state, const char *url, uint8_t wifi_only, void (*callback)(State *state, const HttpResponseMessage *response));

#endif //CARCOMPUTER_SERVER_H
