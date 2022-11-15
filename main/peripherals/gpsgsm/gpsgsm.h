//
// Created by samuel on 23-8-22.
//

#ifndef APP_TEMPLATE_GPSGSM_H
#define APP_TEMPLATE_GPSGSM_H

#include "../../state.h"

void gpsgsm_init();

void gpsgsm_process(State *state);

void gsm_send_sms(const char *number, const char *message);

void gsm_http_post(State *state, const char *url, const char *json);

#endif //APP_TEMPLATE_GPSGSM_H
