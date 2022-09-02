//
// Created by samuel on 4-8-22.
//

#ifndef APP_TEMPLATE_SERVER_H
#define APP_TEMPLATE_SERVER_H

#include "../state.h"

int server_send_trip_end(State *state);

int server_send_data_log_record(State *state);

int server_send_data(State *state, const char *url, const char *json);

#endif //APP_TEMPLATE_SERVER_H
