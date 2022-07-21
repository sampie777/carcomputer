//
// Created by samuel on 20-7-22.
//

#include <stdbool.h>
#include "gas_pedal.h"
#include "../return_codes.h"

int is_pedal_connected() {
    return true;
}

int gas_pedal_read(double *value) {
    if (!is_pedal_connected()) {
        return RESULT_DISCONNECTED;
    }

    return RESULT_OK;
}

void gas_pedal_write(double value) {

}
