//
// Created by samuel on 18-11-22.
//

#include <string.h>
#include <nvs_flash.h>
#include "auth.h"
#include "../return_codes.h"
#include "server.h"
#include "../utils.h"

#define NVS_ACCESS_CODE_KEY "access_code"

static nvs_handle_t handle;

void erase_access_code() {
    printf("Erasing access code value in NVS... ");
    esp_err_t err = nvs_erase_key(handle, NVS_ACCESS_CODE_KEY);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    nvs_erase_key(handle, NVS_ACCESS_CODE_KEY);

    printf("Committing updates in NVS... ");
    err = nvs_commit(handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
}

void store_access_code(const char *value) {
    printf("Updating access code value in NVS to: '%s'... ", value);
    esp_err_t err = nvs_set_str(handle, NVS_ACCESS_CODE_KEY, value);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    printf("Committing updates in NVS... ");
    err = nvs_commit(handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
}

int read_access_code(char **result, size_t *length) {
    printf("Reading access code from NVS... ");
    esp_err_t err = nvs_get_str(handle, NVS_ACCESS_CODE_KEY, NULL, length);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        printf("The value is not initialized yet\n");
        return RESULT_EMPTY;
    }

    if (err != ESP_OK) {
        printf("Error (%s) reading!\n", esp_err_to_name(err));
        return RESULT_FAILED;
    }

    *result = malloc(*length + 1);
    err = nvs_get_str(handle, NVS_ACCESS_CODE_KEY, *result, length);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        printf("The value is not initialized yet\n");
        return RESULT_EMPTY;
    }

    if (err != ESP_OK) {
        printf("Error (%s) reading!\n", esp_err_to_name(err));
        return RESULT_FAILED;
    }

    printf("OK\n");
    return RESULT_OK;
}

void on_registration_token_received(State *state, const HttpResponseMessage *response) {
    printf("[Auth] Processing registration token response\n");
    printf("RESPONSE (token): '%s' (HTTP %d)\n", response->message, response->code);

    if (response->code != 200) return;
    char *token = response->message;

    if (strlen(token) == 0) return;

    state->server.registration_token = malloc(strlen(token) + 1);
    strcpy(state->server.registration_token, token);
}

void on_registration_status_received(State *state, const HttpResponseMessage *response) {
    printf("[Auth] Processing access code response\n");
    printf("RESPONSE (status): '%s' (HTTP %d)\n", response->message, response->code);
    state->server.is_registration_status_check_in_process = false;

    if (response->code != 200) return;
    char *code = response->message;

    if (strlen(code) == 0) return;

    store_access_code(code);
    state->server.access_code = malloc(strlen(code) + 1);
    strcpy(state->server.access_code, code);

    state->server.is_authenticated = true;
    state->server.should_authenticate = false;
    free(state->server.registration_token);
    printf("[Auth] Authentication successful\n");
}

void request_registration_token(State *state) {
    static bool is_requested = false;
    if (is_requested) return;

    printf("[Auth] Requesting registration token...\n");

    char *http_request_url = malloc(strlen(BACKEND_REGISTRATION_TOKEN_URL) + strlen(SERVER_API_KEY) + 16);
    sprintf(http_request_url, "%s%capi_key=%s",
            BACKEND_REGISTRATION_TOKEN_URL,
            strstr(BACKEND_REGISTRATION_TOKEN_URL, "?") == NULL ? '?' : '&',
            SERVER_API_KEY);

    int result = server_receive_data(state, http_request_url, false, on_registration_token_received);
    free(http_request_url);

    if (result == RESULT_FAILED) {
        state->server.should_authenticate = false;
    }
    is_requested = result == RESULT_OK;
}

void poll_registration_status(State *state) {
    static int64_t last_poll_time = 0;

    // Force time out
    if (last_poll_time == 0)
        last_poll_time = esp_timer_get_time_ms();

    if (state->server.is_registration_status_check_in_process || esp_timer_get_time_ms() < last_poll_time + 3000)
        return;
    last_poll_time = esp_timer_get_time_ms();

    printf("[Auth] Checking access code status...\n");

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

    if (!server_is_ready_for_connections(state)) return;

    if (state->server.registration_token == NULL || strlen(state->server.registration_token) == 0) {
        request_registration_token(state);
        return;
    }

    if (state->server.access_code == NULL || strlen(state->server.access_code) == 0) {
        poll_registration_status(state);
        return;
    }
}

void auth_init(State *state) {
    printf("[Auth] Initializing...\n");

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    printf("Opening Non-Volatile Storage (NVS) handle... \n");
    err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }

    size_t length;
    int result = read_access_code(&(state->server.access_code), &length);
    state->server.is_authenticated = result == RESULT_OK && length > 0;
    state->server.should_authenticate = result == RESULT_EMPTY || length <= 0;

    if (state->server.is_authenticated) {
        printf("[Auth] Client is authenticated\n");
    }
}
