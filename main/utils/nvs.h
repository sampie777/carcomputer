//
// Created by samuel on 25-11-22.
//

#ifndef CARCOMPUTER_NVS_H
#define CARCOMPUTER_NVS_H

int nvs_init();

void nvc_erase_all();

void nvs_store_access_token(const char *value);

void nvs_store_device_name(const char *value);

int nvs_read_access_token(char **result, size_t *length);

int nvs_read_device_name(char **result, size_t *length);

#endif //CARCOMPUTER_NVS_H
