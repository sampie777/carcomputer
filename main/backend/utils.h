//
// Created by samuel on 23-11-22.
//

#ifndef CARCOMPUTER_UTILS_H
#define CARCOMPUTER_UTILS_H

#include <stdint-gcc.h>
#include <stddef.h>

char letter_from_int(uint32_t input);

void generate_registration_token(char *token, size_t length);

#endif //CARCOMPUTER_UTILS_H
