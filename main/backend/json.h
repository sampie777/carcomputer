//
// Created by samuel on 25-11-22.
//

#ifndef CARCOMPUTER_JSON_H
#define CARCOMPUTER_JSON_H

typedef struct {
    char *access_token;
    char *device_name;
} RegistrationResponse;

void parse_json_to_registration_response(const char *json, RegistrationResponse *output);

#endif //CARCOMPUTER_JSON_H
