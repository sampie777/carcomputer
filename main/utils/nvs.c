//
// Created by samuel on 25-11-22.
//

#include <nvs.h>
#include <nvs_flash.h>
#include "nvs.h"
#include "../return_codes.h"

#define NVS_ACCESS_TOKEN_KEY "access_token"
#define NVS_DEVICE_NAME_KEY "device_name"

static nvs_handle_t handle;

void erase_nvs_key(const char *key) {
    printf("Erasing '%s' value in NVS... ", key);
    esp_err_t err = nvs_erase_key(handle, key);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    printf("Committing updates in NVS... ");
    err = nvs_commit(handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
}

void erase_all() {
    erase_nvs_key(NVS_ACCESS_TOKEN_KEY);
    erase_nvs_key(NVS_DEVICE_NAME_KEY);
}

void store_nvs_key(const char *key, const char *value) {
    printf("Updating '%s' value in NVS to: '%s'... ", key, value);
    esp_err_t err = nvs_set_str(handle, key, value);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    printf("Committing updates in NVS... ");
    err = nvs_commit(handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
}

void nvs_store_access_token(const char *value) {
    store_nvs_key(NVS_ACCESS_TOKEN_KEY, value);
}

void nvs_store_device_name(const char *value) {
    store_nvs_key(NVS_DEVICE_NAME_KEY, value);
}

int read_nvs_key(const char *key, char **result, size_t *length) {
    printf("Reading '%s' from NVS... ", key);
    esp_err_t err = nvs_get_str(handle, key, NULL, length);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        printf("The value is not initialized yet\n");
        return RESULT_EMPTY;
    }

    if (err != ESP_OK) {
        printf("Error (%s) reading!\n", esp_err_to_name(err));
        return RESULT_FAILED;
    }

    *result = malloc(*length + 1);
    err = nvs_get_str(handle, key, *result, length);

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

int nvs_read_access_token(char **result, size_t *length) {
    return read_nvs_key(NVS_ACCESS_TOKEN_KEY, result, length);
}

int nvs_read_device_name(char **result, size_t *length) {
    return read_nvs_key(NVS_DEVICE_NAME_KEY, result, length);
}

int nvs_init() {
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
        return RESULT_FAILED;
    }
    return RESULT_OK;
}