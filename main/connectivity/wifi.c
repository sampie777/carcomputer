//
// Created by samuel on 17-7-22.
//

#include <stdio.h>
#include "wifi.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "../config.h"

#define DEFAULT_RSSI -127
#define DEFAULT_AUTHMODE WIFI_AUTH_WPA2_PSK

static const char *TAG = "scan";

/**
 *
 * @param args Should be a pointer to the structure State
 * @param event_base
 * @param event_id
 * @param event_data
 */
static void event_handler(void *args, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
    State *state = args;

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // When is connected with IP
        printf("[WiFi] IP_EVENT_STA_GOT_IP\n");
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        state->wifi.connected = true;
        state->wifi.ip = event->ip_info.ip;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("[WiFi] WIFI_EVENT_STA_DISCONNECTED\n");
        state->wifi.connected = false;
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
        printf("[WiFi] IP_EVENT_STA_LOST_IP\n");
        state->wifi.connected = false;
    }
}

void fast_scan(State *state) {
    printf("Setting up WiFi connection with AP: %s...\n", DEFAULT_SSID);
    // See: https://github.com/espressif/esp-idf/blob/v4.4.1/examples/wifi/fast_scan/main/fast_scan.c
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &event_handler, state, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &event_handler, state, NULL));

    // Initialize default station as network interface instance (esp-netif)
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // Initialize and start WiFi
    wifi_config_t wifi_config = {
            .sta = {
                    .ssid = DEFAULT_SSID,
                    .password = DEFAULT_PWD,
                    .scan_method = WIFI_FAST_SCAN,
                    .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                    .threshold.rssi = DEFAULT_RSSI,
                    .threshold.authmode = DEFAULT_AUTHMODE,
            },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    printf("Starting WiFi scan...\n");
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_connect(State *state) {
    fast_scan(state);
}