//
// Created by samuel on 23-8-22.
//

#ifndef APP_TEMPLATE_GPSGSM_H
#define APP_TEMPLATE_GPSGSM_H

#include "../../state.h"

#define A9G_UART_BUFFER_SIZE (1024)

extern QueueHandle_t uart_queue;

void gpsgsm_init(A9GState *a9g_state);

void gpsgsm_process(State *state);

void gsm_send_sms(const char *number, const char *message);

// void gsm_http_get(State *state, const char *url, void (*callback)(State *state, const HttpResponseMessage *response));

// void gsm_http_post(State *state, const char *url, const char *json);

#endif //APP_TEMPLATE_GPSGSM_H
