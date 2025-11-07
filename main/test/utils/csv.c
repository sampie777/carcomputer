//
// Created by Samuel-Anton Jansen on 2025/11/07.
//

#include "csv.h"
#include <stdio.h>

const char* file_name = "../../../test1.csv";

int write_csv() {
    FILE* file = fopen(file_name, "w");
    if (file == NULL) {
        printf("ERROR: Could not open file %s for writing\n", file_name);
        return 1;
    }

    fwrite("", sizeof(char), 0, file);
    fclose(file);
    return 0;
}

int append_csv(const char* data, unsigned long size) {
    if (!data) {
        printf("ERROR: Input BMP_File pointer is NULL\n");
        return 1;
    }

    FILE* file = fopen(file_name, "a+");
    if (file == NULL) {
        printf("ERROR: Could not open file %s for writing\n", file_name);
        return 1;
    }

    fwrite(data, sizeof(char), size, file);
    fclose(file);
    printf("%s", data);
    return 0;
}