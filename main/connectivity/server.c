//
// Created by samuel on 4-8-22.
//

#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_tls.h>
#include "server.h"
#include "../utils.h"
#include "../return_codes.h"

// Source: https://github.com/espressif/esp-idf/blob/36f49f361c001b49c538364056bc5d2d04c6f321/examples/protocols/esp_http_client/main/esp_http_client_example.c

#define MAX_HTTP_OUTPUT_BUFFER 1024
static const char *TAG = "Server";

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

int server_send_data(char *data) {
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    esp_http_client_config_t config = {
            .url = SERVER_POST_URL,
            .event_handler = server_http_event_handler,
            .user_data = local_response_buffer,
            .disable_auto_redirect = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_url(client, SERVER_POST_URL);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, data, (int) strlen(data));

    printf("[Server] Performing request to %s...\n", config.url);
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
}

int server_send_trip_end(State *state) {
    printf("[Server] Logging trip end...\n");
    state->trip_is_uploading = true;
    char buffer[512];
    sprintf(buffer, "{"
                    "\"uptimeMs\": \"%lld\","
                    "\"wifiSsid\": \"%s\","
                    "\"odometerStart\": %d,"
                    "\"odometerEnd\": %d"
                    "}", esp_timer_get_time_ms(), state->wifi.ssid, state->car.odometer_start, state->car.odometer_end);
    int result = server_send_data(buffer);
    state->trip_is_uploading = false;
    return result;
}