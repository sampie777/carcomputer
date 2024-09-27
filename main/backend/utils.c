//
// Created by samuel on 23-11-22.
//

#include <esp_random.h>
#include <bootloader_random.h>
#include "utils.h"

char letter_from_int(uint32_t input) {
    int char_code = 65 + (int) ((double) input / UINT32_MAX * 25);
    return (char) char_code;
}

void generate_registration_token(char *token, size_t length) {
    bootloader_random_enable();
    for (int i = 0; i < length; i++) {
        if (i < length - 1 && i % 4 == 3) {
            token[i] = '-';
        } else {
            token[i] = letter_from_int(esp_random());
        }
    }
    bootloader_random_disable();

    token[length] = '\0';
}

uint32_t generate_session_id() {
    bootloader_random_enable();
    uint32_t id = esp_random();
    bootloader_random_disable();
    return id;
}