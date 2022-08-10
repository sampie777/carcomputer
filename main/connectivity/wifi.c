//
// Created by samuel on 17-7-22.
//

#include <stdio.h>
#include <memory.h>
#include "../config.h"
#include "wifi.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "../utils.h"

#define RSSI_MIN_THRESHOLD -127

static const char *TAG = "WiFi";

static WiFiAPCredentials *wifi_known_aps;
static int wifi_known_aps_length = -1;

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

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        state->wifi.has_scan_results = true;
        printf("[WiFi] Scan done\n");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // When is connected with IP
        printf("[WiFi] IP_EVENT_STA_GOT_IP\n");
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));

        state->wifi.is_connected = true;
        state->wifi.is_connecting = false;
        state->wifi.ip = event->ip_info.ip;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        printf("[WiFi] WIFI_EVENT_STA_START\n");
        if (state->wifi.is_connecting || state->wifi.is_connected) {
            esp_wifi_connect();
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_event_sta_connected_t *event = ( wifi_event_sta_connected_t *) event_data;
        printf("[WiFi] WIFI_EVENT_STA_CONNECTED to %s\n", event->ssid);
        memcpy(state->wifi.ssid, event->ssid, event->ssid_len);
        state->wifi.ssid[min(31, event->ssid_len)] = '\0';
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("[WiFi] WIFI_EVENT_STA_DISCONNECTED\n");
        state->wifi.is_connected = false;
        state->wifi.is_connecting = false;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
        printf("[WiFi] IP_EVENT_STA_LOST_IP\n");
        state->wifi.is_connected = false;
        state->wifi.is_connecting = false;
    }
}

void connect_with_ap(State *state, WiFiAPCredentials *credentials) {
    printf("[WiFi] Setting up WiFi connection with AP: %s...\n", credentials->ssid);

    // See: https://github.com/espressif/esp-idf/blob/v4.4.1/examples/wifi/fast_scan/main/fast_scan.c
    // Initialize and start WiFi
    wifi_config_t wifi_config = {
            .sta = {
                    .scan_method = WIFI_FAST_SCAN,
                    .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                    .threshold.rssi = RSSI_MIN_THRESHOLD,
                    .threshold.authmode = credentials->authmode,
            },
    };
    memcpy(wifi_config.sta.ssid, credentials->ssid, 32);
    memcpy(wifi_config.sta.password, credentials->password, 64);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    printf("[WiFi] Starting WiFi connection... \n");
    state->wifi.is_connecting = true;
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_stop_scan(State *state) {
    state->wifi.is_scanning = false;
    state->wifi.has_scan_results = false;
}

void process_scan_results(State *state) {
    printf("[WiFi] Processing scan results...\n");

    uint16_t ap_count = 0;
    uint16_t number = WIFI_SCAN_MAX_APS;
    wifi_ap_record_t ap_info[WIFI_SCAN_MAX_APS] = {0};

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    esp_wifi_stop();
    wifi_stop_scan(state);

    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
    for (int i = 0; (i < WIFI_SCAN_MAX_APS) && (i < ap_count); i++) {
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);

        // Connect to known AP's
        for (int j = 0; j < wifi_known_aps_length; j++) {
            if (strcmp((char *) ap_info[i].ssid, (char *) wifi_known_aps[j].ssid) != 0) {
                continue;
            }

            wifi_known_aps[j].authmode = ap_info[i].authmode;
            connect_with_ap(state, &wifi_known_aps[j]);
            return;
        }

        // Else, connect to free WiFi
        for (int j = 0; j < wifi_known_aps_length; j++) {
            if (ap_info[i].authmode != WIFI_AUTH_OPEN) {
                continue;
            }

            WiFiAPCredentials credentials = {0};
            memcpy(credentials.ssid, ap_info[i].ssid, 32);
            credentials.authmode = ap_info[i].authmode;

            connect_with_ap(state, &credentials);
            return;
        }
    }
    printf("[WiFi] No AP found to connect with\n");
}

void wifi_scan(State *state) {
    static int64_t last_scan_time = 0;
    static int64_t scan_start_time = 0;

    if (state->wifi.has_scan_results) {
        process_scan_results(state);
        last_scan_time = esp_timer_get_time_ms();
    }

    if (state->wifi.is_connecting || state->wifi.is_connected) {
        wifi_stop_scan(state);
        return;
    }

    if (state->car.speed >= 1 || state->car.rpm >= 1) {
        esp_wifi_scan_stop();
        wifi_stop_scan(state);
        return;
    }

    if (esp_timer_get_time_ms() < last_scan_time + WIFI_SCAN_INTERVAL_MS) {
        return;
    }

    if (state->wifi.is_scanning) {
        printf("[WiFi] Still scanning...\n");

        if (esp_timer_get_time_ms() > scan_start_time + WIFI_SCAN_MAX_DURATION) {
            printf("[WiFi] Scan timeout\n");
            esp_wifi_scan_stop();
            wifi_stop_scan(state);
        }
    } else {
        printf("[WiFi] Starting scan...\n");
        ESP_ERROR_CHECK(esp_wifi_start());

        state->wifi.is_scanning = true;
        scan_start_time = esp_timer_get_time_ms();
        esp_wifi_scan_start(NULL, false);
    }
    last_scan_time = esp_timer_get_time_ms();
}

void wifi_store_known_ap(WiFiAPCredentials credentials) {
    if (wifi_known_aps_length == -1) {
        wifi_known_aps = malloc(0);
        wifi_known_aps_length = 0;
    }

    printf("[WiFi] Storing new WiFi AP: %s\n", credentials.ssid);
    wifi_known_aps = realloc(wifi_known_aps, ++wifi_known_aps_length * sizeof(WiFiAPCredentials));
    wifi_known_aps[wifi_known_aps_length - 1] = credentials;
}

void wifi_init(State *state) {
    printf("[WiFi] Initializing...\n");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize default station as network interface instance (esp-netif)
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(sta_netif, DEVICE_NAME);
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    esp_event_loop_create_default();
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, state, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, state, NULL));

    wifi_store_known_ap((WiFiAPCredentials) {
            .ssid = DEFAULT_SSID,
            .password = DEFAULT_PWD,
    });
    wifi_store_known_ap((WiFiAPCredentials) {
            .ssid = DEFAULT_SSID1,
            .password = DEFAULT_PWD1,
    });

    printf("[WiFi] Init done\n");
}
