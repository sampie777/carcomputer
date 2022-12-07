//
// Created by samuel on 18-11-22.
//

#include <string.h>
#include "auth.h"
#include "../return_codes.h"
#include "server.h"
#include "../utils.h"
#include "utils.h"
#include "json.h"
#include "../utils/nvs.h"

void on_registration_status_received(State *state, const HttpResponseMessage *response) {
    printf("[Auth] Processing access token response\n");
    printf("RESPONSE (status): '%s' (HTTP %d)\n", response->message, response->code);
    state->server.is_registration_status_check_in_process = false;

    if (response->code != 200) return;

    if (strlen(response->message) == 0) return;

    RegistrationResponse response_struct = {};
    parse_json_to_registration_response(response->message, &response_struct);

    nvs_store_access_token(response_struct.access_token);
    state->server.access_token = malloc(strlen(response_struct.access_token) + 1);
    strcpy(state->server.access_token, response_struct.access_token);

    nvs_store_device_name(response_struct.device_name);
    state->device_name = malloc(strlen(response_struct.device_name) + 1);
    strcpy(state->device_name, response_struct.device_name);

    state->server.is_authenticated = true;
    state->server.should_authenticate = false;
    free(state->server.registration_token);
    printf("[Auth] Authentication successful\n");
}

void poll_registration_status(State *state) {
    static int64_t last_poll_time = 0;

    // Force time out
    if (last_poll_time == 0)
        last_poll_time = esp_timer_get_time_ms();

    if (state->server.is_registration_status_check_in_process || esp_timer_get_time_ms() < last_poll_time + 3000)
        return;
    last_poll_time = esp_timer_get_time_ms();

    printf("[Auth] Checking access token status...\n");

    char *http_request_url = malloc(strlen(BACKEND_REGISTRATION_STATUS_URL) + strlen(SERVER_API_KEY) + strlen(state->server.registration_token) + 32);
    sprintf(http_request_url, "%s%capi_key=%s&token=%s",
            BACKEND_REGISTRATION_STATUS_URL,
            strstr(BACKEND_REGISTRATION_STATUS_URL, "?") == NULL ? '?' : '&',
            SERVER_API_KEY,
            state->server.registration_token);

    int result = server_receive_data(state, http_request_url, false, on_registration_status_received);
    free(http_request_url);

    if (result == RESULT_FAILED) {
        state->server.should_authenticate = false;
    }

    state->server.is_registration_status_check_in_process = result == RESULT_OK;
}

void auth_process(State *state) {
    if (state->server.is_authenticated) return;
    if (!state->server.should_authenticate) return;

    if (state->server.registration_token == NULL || strlen(state->server.registration_token) == 0) {
        printf("[Auth] Generating registration token... ");
        state->server.registration_token = malloc(BACKEND_REGISTRATION_TOKEN_LENGTH + 1);
        generate_registration_token(state->server.registration_token, BACKEND_REGISTRATION_TOKEN_LENGTH);
        printf("%s\n", state->server.registration_token);
        return;
    }

    if (!server_is_ready_for_connections(state)) return;

    if (state->server.access_token == NULL || strlen(state->server.access_token) == 0) {
        poll_registration_status(state);
        return;
    }
}

void auth_init(State *state) {
    printf("[Auth] Initializing...\n");

    size_t length;
    int result = nvs_read_access_token(&(state->server.access_token), &length);
    state->server.is_authenticated = result == RESULT_OK && length > 0;
    state->server.should_authenticate = result == RESULT_EMPTY || length <= 0;

    if (state->server.is_authenticated) {
        printf("[Auth] Client is authenticated\n");
    }
}
