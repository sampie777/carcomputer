//
// Created by samuel on 4-8-22.
//

#include <esp_err.h>
#include <string.h>
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_tls.h>
#include "server.h"
#include "../return_codes.h"
#include "../utils.h"
#include "auth.h"

#if GSM_ENABLE

#include "../peripherals/gpsgsm/gpsgsm.h"

#endif

#define MAX_HTTP_OUTPUT_BUFFER 512
static const char *TAG = "Server";

// Source: https://github.com/espressif/esp-idf/blob/36f49f361c001b49c538364056bc5d2d04c6f321/examples/protocols/esp_http_client/main/esp_http_client_example.c
esp_err_t server_http_event_handler(esp_http_client_event_t *evt) {
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t) evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
    }
    return ESP_OK;
}

int send_data_over_wifi(State *state, const char *url, const char *data) {
#if WIFI_ENABLE
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

    char *http_request_url = malloc(strlen(url) + strlen(state->server.access_code)+ strlen(SERVER_API_KEY) + 32);
    sprintf(http_request_url, "%s%capi_key=%s&access_token=%s", url, strstr(url, "?") == NULL ? '?' : '&', SERVER_API_KEY, state->server.access_token);

    esp_http_client_config_t config = {
            .url = http_request_url,
            .event_handler = server_http_event_handler,
            .user_data = local_response_buffer,
            .disable_auto_redirect = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    char authorization_header_value[strlen(state->server.access_code) + 10];
    sprintf(authorization_header_value, "Bearer %s", state->server.access_code);

    esp_http_client_set_url(client, config.url);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, data, (int) strlen(data));

    printf("[Server] Performing POST request to %s...\n", config.url);
    int result = esp_http_client_perform(client);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(result));
    } else {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }

    printf("[Server] Output: '");
    for (int i = 0; i < strlen(local_response_buffer); i++) {
        printf("%c", local_response_buffer[i]);
    }
    printf("'\n");

    esp_http_client_cleanup(client);

    if (result == ESP_OK) {
        return RESULT_OK;
    }
    return RESULT_FAILED;
#else
    return RESULT_FAILED;
#endif
}

int receive_data_over_wifi(State *state, const char *url, void (*callback)(State *state, const HttpResponseMessage *response)) {
#if WIFI_ENABLE
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

    esp_http_client_config_t config = {
            .url = url,
            .event_handler = server_http_event_handler,
            .user_data = local_response_buffer,
            .disable_auto_redirect = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_url(client, url);
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    printf("[Server] Performing GET request to %s...\n", config.url);
    int result = esp_http_client_perform(client);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(result));
    } else {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }

    printf("[Server] Output: '");
    for (int i = 0; i < strlen(local_response_buffer); i++) {
        printf("%c", local_response_buffer[i]);
    }
    printf("'\n");

    esp_http_client_cleanup(client);

    if (result != ESP_OK) return RESULT_FAILED;
    if (callback != NULL) {
        HttpResponseMessage response = {
                .code = esp_http_client_get_status_code(client),
                .content_length = esp_http_client_get_content_length(client),
                .message = malloc(strlen(local_response_buffer) + 1),
        };
        strcpy(response.message, local_response_buffer);

        callback(state, &response);
        free(response.message);
    };

    return RESULT_OK;
#else
    return RESULT_FAILED;
#endif
}

int send_data_over_gsm(State *state, const char *url, const char *data) {
#if GSM_ENABLE
    gsm_http_post(state, url, data);
#endif
    return RESULT_OK;
}

int receive_data_over_gsm(State *state, const char *url, void (*callback)(State *state, const HttpResponseMessage *response)) {
#if GSM_ENABLE
    gsm_http_get(state, url, callback);
#endif
    return RESULT_OK;
}

int send_data(State *state, const char *url, const char *data, uint8_t wifi_only) {
    if (wifi_only && !state->wifi.is_connected)
        return RESULT_DISCONNECTED;

    if (state->wifi.is_connected)
        return send_data_over_wifi(state, url, data);
    return send_data_over_gsm(state, url, data);
}

int receive_data(State *state, const char *url, uint8_t wifi_only, void (*callback)(State *state, const HttpResponseMessage *response)) {
    if (wifi_only && !state->wifi.is_connected)
        return RESULT_DISCONNECTED;

    if (state->wifi.is_connected)
        return receive_data_over_wifi(state, url, callback);
    return receive_data_over_gsm(state, url, callback);
}

int server_send_trip_end(State *state) {
    printf("[Server] Logging trip end...\n");

    char timestamp[64];
    Time time = state->location.time.year > 2021 ? state->location.time : state->gsm.time;
    if (time.year < 2000) {
        timestamp[0] = '\0';
    } else {
        sprintf(timestamp, ",\"time\":\"%04d-%02d-%02d'T'%02d:%02d:%02d.000%+d\"",
                time.year,
                time.month,
                time.day,
                time.hours,
                time.minutes,
                time.seconds,
                time.timezone);
    }

    char location[128];
    if (state->location.satellites == 0) {
        location[0] = '\0';
    } else {
        sprintf(location, ",\"quality\":%d,"
                          "\"is_effective_positioning\":%d,"
                          "\"latitude\":%.5f,"
                          "\"longitude\":%.5f,"
                          "\"altitude\":%.1f",
                state->location.quality,
                state->location.is_effective_positioning,
                state->location.latitude,
                state->location.longitude,
                state->location.altitude
        );
    }

    char buffer[512];
    sprintf(buffer, "{"
                    "\"uptimeMs\":%lld,"
                    "\"session\":%u,"
                    #if WIFI_ENABLE
                    "\"wifi\":{"
                    """\"ssid\":\"%s\""
                    "},"
                    #endif
                    "\"car\":{"
                    """\"odometer_start\":%d,"
                    """\"odometer\":%d"
                    "},"
                    "\"location\":{"
                    """\"is_gps_on\":%d,"
                    """\"satellites\":%d"
                    """%s"
                    """%s"
                    "}"
                    "}",
            esp_timer_get_time_ms(),
            state->logging_session_id,

#if WIFI_ENABLE
            state->wifi.ssid,
#endif
            state->car.odometer_start,
            state->car.odometer,
            state->location.is_gps_on,
            state->location.satellites,
            location,
            timestamp);
    return server_send_data(state, TRIP_LOGGER_UPLOAD_URL_TRIP_END, buffer, false);
}

int server_send_data_log_record(State *state) {
    printf("[Server] Logging data record...\n");

    char timestamp[64];
    Time time = state->location.time.year > 2021 ? state->location.time : state->gsm.time;
    if (time.year < 2000) {
        timestamp[0] = '\0';
    } else {
        sprintf(timestamp, ",\"time\":\"%04d-%02d-%02d'T'%02d:%02d:%02d.000%+d\"",
                time.year,
                time.month,
                time.day,
                time.hours,
                time.minutes,
                time.seconds,
                time.timezone);
    }

    char buffer[1200];
    sprintf(buffer, "{"
                    "\"uptimeMs\":%lld,"
                    "\"session\":%u,"
                    "\"car\":{"
                    """\"is_connected\":%d,"
                    """\"is_controller_connected\":%d,"
                    """\"is_braking\":%d,"
                    """\"is_ignition_on\":%d,"
                    """\"speed\":%.3f,"
                    """\"rpm\":%.3f,"
                    """\"odometer\":%d,"
                    """\"gas_pedal_connected\":%d,"
                    """\"gas_pedal\":%.5f"
                    "},"
                    #if CRUISE_CONTROL_ENABLE
                    "\"cruiseControl\":{"
                    """\"enabled\":%d,"
                    """\"target_speed\":%.3f,"
                    """\"virtual_gas_pedal\":%.5f,"
                    """\"control_value\":%.5f"
                    "},"
                    #endif
                    #if WIFI_ENABLE
                    "\"wifi\":{"
                    """\"ssid\":\"%s\","
                    """\"ip\":%d,"
                    """\"is_connected\":%d"
                    "},"
                    #endif
                    #if BLUETOOTH_ENABLE
                    "\"bluetooth\":{"
                    """\"connected\":%d"
                    "},"
                    #endif
                    "\"motion\":{"
                    """\"connected\":%d,"
                    """\"accel_x\":%.3f,"
                    """\"accel_y\":%.3f,"
                    """\"accel_z\":%.3f,"
                    """\"gyro_x\":%.3f,"
                    """\"gyro_y\":%.3f,"
                    """\"gyro_z\":%.3f,"
                    """\"compass_x\":%.3f,"
                    """\"compass_y\":%.3f,"
                    """\"compass_z\":%.3f,"
                    """\"temperature\":%.3f"
                    "},"
                    "\"location\":{"
                    """\"is_gps_on\":%d,"
                    """\"quality\":%d,"
                    """\"satellites\":%d,"
                    """\"is_effective_positioning\":%d,"
                    """\"latitude\":%.5f,"
                    """\"longitude\":%.5f,"
                    """\"altitude\":%.1f,"
                    """\"ground_speed\":%.3f,"
                    """\"ground_heading\":%.2f"
                    "%s"
                    "}"
                    "}\n",
            esp_timer_get_time_ms(),
            state->logging_session_id,

            state->car.is_connected,
            state->car.is_controller_connected,
            state->car.is_braking,
            state->car.is_ignition_on,
            state->car.speed,
            state->car.rpm,
            state->car.odometer,
            state->car.gas_pedal_connected,
            state->car.gas_pedal,

#if CRUISE_CONTROL_ENABLE
            state->cruise_control.enabled,
            state->cruise_control.target_speed,
            state->cruise_control.virtual_gas_pedal,
            state->cruise_control.control_value,
#endif
#if WIFI_ENABLE
            state->wifi.ssid,
            state->wifi.ip.addr,
            state->wifi.is_connected,
#endif
#if BLUETOOTH_ENABLE
            state->bluetooth.connected,
#endif

            state->motion.connected,
            state->motion.accel_x,
            state->motion.accel_y,
            state->motion.accel_z,
            state->motion.gyro_x,
            state->motion.gyro_y,
            state->motion.gyro_z,
            state->motion.compass_x,
            state->motion.compass_y,
            state->motion.compass_z,
            state->motion.temperature,

            state->location.is_gps_on,
            state->location.quality,
            state->location.satellites,
            state->location.is_effective_positioning,
            state->location.latitude,
            state->location.longitude,
            state->location.altitude,
            state->location.ground_speed,
            state->location.ground_heading,
            timestamp
    );
    return server_send_data(state, DATA_LOGGER_UPLOAD_URL_FULL_DATA, buffer, false);
}

int server_send_data(State *state, const char *url, const char *json, uint8_t wifi_only) {
    if (!state->server.is_authenticated) {
        printf("[Server] Cannot perform request: not authenticated\n");
        return RESULT_UNAUTHORIZED;
    }

    state->server_is_uploading = true;
    int result = send_data(state, url, json, wifi_only);
    state->server_is_uploading = false;
    return result;
}

int server_receive_data(State *state, const char *url, uint8_t wifi_only, void (*callback)(State *state, const HttpResponseMessage *response)) {
    state->server_is_uploading = true;
    int result = receive_data(state, url, wifi_only, callback);
    state->server_is_uploading = false;
    return result;
}

bool server_is_ready_for_connections(State *state) {
    return state->wifi.is_connected || state->a9g.pnp_activated == A9Status_Ok;
}

void server_process(State *state) {
    auth_process(state);
}

void server_init(State *state) {
    auth_init(state);
}
